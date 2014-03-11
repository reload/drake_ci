<?php

/**
 * @file
 * Run simpletests from the specified files.
 *
 * Runs an internal server, installs a site and runs the tests found in the
 * given files.
 */

/**
 * Task.
 */
$tasks['run-simpletests'] = array(
  'action' => 'run-simpletests',
  'files' => fileset('php-custom'),
  'output-dir' => context_optional('output-dir'),
  'port' => context_optional('port'),
  'no-cleanup' => context_optional('no-cleanup'),
  'tests' => context_optional('tests'),
  'abort-on-failure' => context_optional('abort-on-failure'),
);

/**
 * Action.
 */
$actions['run-simpletests'] = array(
  'default_message' => 'Simpletests.',
  'callback' => 'drake_ci_run_simpletests',
  'parameters' => array(
    'files' => 'Files to check for tests.',
    'output-dir' => array(
      'description' => 'Output CSV files here.',
      'default' => '',
    ),
    'port' => array(
      'description' => 'Port number to use for local server.',
      'default' => '',
    ),
    'no-cleanup' => array(
      'description' => 'Do not remove installed test site.',
      'default' => '',
    ),
    'tests' => array(
      'description' => 'Specific tests to run. If not specified, tests found in files will be run.',
      'default' => array(),
    ),
    'abort-on-failure' => array(
      'description' => 'Whether to abort on first failure.',
      'default' => FALSE,
    ),
  ),
);


/**
 * Action callback; run simpletests for files.
 */
function drake_ci_run_simpletests($context) {
  // If port was not specified, pick a random port. With so many to choose from,
  // we're unlikely to collide.
  $port = !empty($context['port']) ? $context['port'] : mt_rand(50000, 60000);
  $cleanup = !((bool) $context['no-cleanup']);
  $site_dir = $port . '.localhost';
  $profile = NULL;
  $tests = $context['tests'];
  if (!is_array($tests)) {
    $tests = explode(",", $tests);
  }
  $default_options = array(
    'uri' => 'http://localhost:' . $port,
  );

  // We need to figure out whether any af the tests we'll run is from a module
  // that's in a profile, as we need to install the site with that profile to
  // run the given test. However, we can't figure out precisely what tests we'll
  // run, as we need an installation to verify that it's an test we'll really
  // run.
  //
  // This is "good enough", it'll simply pick a profile if we found any tests
  // that was located in a profile. It'll fail if it finds tests in multiple
  // profiles, but that's very unlikely to happen in real usage anyway, and then
  // the user will just have to be more explicit.
  $potential_tests = array();
  if (empty($tests)) {
    // Find tests to run by greping all selected files for class names, and
    // taking all that's also listed in the drush test-run listing.
    foreach ($context['files'] as $file) {
      // Only bother with test files.
      // @todo will need to be adjusted for D8.
      if (preg_match('/.test$/', $file->path())) {
        drush_shell_exec('grep "class " %s', $file->fullPath());
        // Grep returns all lines that contains "class ", filter it down a bit and
        // get the class names.
        if (preg_match_all('/^\s*(?:abstract|final)?\s*class\s+(\S+)[^{]*{/m', implode("\n", drush_shell_exec_output()), $matches, PREG_PATTERN_ORDER)) {
          foreach ($matches[1] as $name) {
            // Figure out if we need a specific profile for the tests.
            if (preg_match('{^profiles/([^/]+)/}', $file->path(), $m)) {
              if (!empty($profile) and $profile != $m[1]) {
                return drake_action_error('Cannot test files from different profiles.');
              }
              $profile = $m[1];
            }
            // Create a lookup table.
            $potential_tests[$name] = TRUE;
          }
        }
      }
    }

    if (empty($potential_tests)) {
      // No tests specified and none found. Return.
      drush_log(dt('No tests found in files, skipping.'), 'status');
      return;
    }
  }

  // Use the testing profile if no specific profile is required.
  if (empty($profile)) {
    $profile = 'testing';
  }

  // Register dir/files for deletion when we're done.
  if ($cleanup) {
    // Delete site dir.
    drush_register_file_for_deletion('sites/' . $site_dir, TRUE);
    // Delete database.
    drush_register_file_for_deletion('sites/' . $site_dir . '.sqlite', TRUE);
  }

  // If the site dir exists, assume we don't need to set it up.
  if (!file_exists('sites/' . $site_dir)) {
    $args = array($profile);
    $options = array(
      // Drupal does not like it when the database is in the site dir, because
      // the installer changes the permissions so SQLite can't create a lock
      // file which makes it fail. So we'll put it outside.
      'db-url' => 'sqlite://sites/' . $site_dir . '.sqlite',
      'sites-subdir' => $site_dir,
    );
    $res = drush_invoke_process(NULL, 'site-install', $args, $options, TRUE);

    if (!$res || $res['error_status'] != 0) {
      return drake_action_error(dt('Error installing site.'));
    }

    // Enable simpletest.module.
    $res = drush_invoke_process(NULL, 'pm-enable', array('simpletest'), $default_options, TRUE);

    if (!$res || $res['error_status'] != 0) {
      return drake_action_error(dt('Error enabling simpletest module.'));
    }
  }

  // Use a temporary log file, to avoid buffers being filled.
  $log_file = '/tmp/simpletest-' . $port . '-' . posix_getpid() . '.log';
  drush_register_file_for_deletion($log_file);
  $descriptorspec = array(
    0 => array('file', '/dev/null', 'r'),
    1 => array('file', '/dev/null', 'w'),
    2 => array('file', $log_file, 'w'),
  );
  // We'd like to use drush runserver instead, but in initial testing runserver
  // would cause core tests to fail on login, while this would not.
  $process = proc_open('php -S localhost:' . $port . ' ' . dirname(dirname(__FILE__)) . '/router.php', $descriptorspec, $pipes);
  if (!$process) {
    return drake_action_error(dt('Could not start internal web server.'));
  }

  // Register a shutdown function to properly close the subprocess.
  register_shutdown_function('_drake_ci_run_simpletests_shutdown', $process);

  // Figure out which of the potential test names is available as tests that can
  // be run.
  if (count($potential_tests)) {
    // Get list of all tests.
    $res = drush_invoke_process(NULL, 'test-run', array(), $default_options + array('pipe' => TRUE), TRUE);

    if (!$res || $res['error_status'] != 0) {
      return drake_action_error(dt('Error getting list af all tests.'));
    }
    // You'd think that using --pipe would be da shizzle, but it's just an
    // array represenation of the table printed, including headers, group
    // headers and formatting spaces.
    // To add insult to injury, different Drush/Drupal versions format the
    // array differently.
    if (isset($res['object']['_data'])) {
      foreach ($res['object']['_data'] as $row) {
        $class_name = trim($row[0]);
        if (isset($potential_tests[$class_name])) {
          $tests[] = $class_name;
        }
      }
    }
    else {
      foreach ($res['object'] as $row) {
        if (isset($row['class'])) {
          if (isset($potential_tests[$row['class']])) {
            $tests[] = $row['class'];
          }
        }
      }
    }
  }

  if (empty($tests)) {
    // No tests to run, exit.
    drush_log(dt('No runnable tests found, skipping.'), 'status');
  }

  $test_errors = FALSE;
  // Run the tests.
  $options = $default_options;
  if (!empty($context['output-dir'])) {
    $dir = $context['output-dir'] . '/xUnit';
    $options += array('xml' => $dir);
    // Clean task should ensure that the output-dir exists.
    if (!file_exists($dir)) {
      mkdir($dir);
    }
  }
  foreach ($tests as $test) {
    $res = drush_invoke_process(NULL, 'test-run', array($test), $options, TRUE);

    if (!$res || $res['error_status'] != 0) {
      $test_errors = TRUE;
      if ($context['abort-on-failure']) {
        return drake_action_error(dt('Error while running test @test, aborting', array('@test' => $test)));
      }
      drush_log(dt('Error while running test @test', array('@test' => $test)), 'error');
    }
  }

  if ($test_errors) {
    // Messages was already logged.
    return FALSE;
  }
}

/**
 * Shutdown function to end the PHP server process.
 */
function _drake_ci_run_simpletests_shutdown($process) {
  // We assume that all is dandy if the server is still running. Can't count on
  // return code from proc_close.
  $proc_status = proc_get_status($process);
  $php_rc = $proc_status["running"] ? 0 : $proc_status["exitcode"];
  if ($php_rc != 0) {
    drush_set_error('PHP_SERVER_ERROR', dt("The PHP server process returned error."));
  }

  if ($proc_status['running'] && $proc_status['pid']) {
    // In some PHP versions, the child process isn't php, but sh. Try to find
    // the child processes of the process and kill them off by hand first.
    $ppid = $proc_status['pid'];
    $pids = array();
    // Get any children of the sub process by asking ps for a list of processes
    // with their parent pid and looking through it for our subprocess. On Linux
    // the right options to ps can give just the children, but this has the
    // advantage of also working on OSX.
    drush_shell_exec("ps -o pid,ppid -ax");
    foreach (drush_shell_exec_output() as $line) {
      if (preg_match('/^\s*([0-9]+)\s*' . $proc_status['pid'] . '\s*$/', $line, $m)) {
        $pids[] = $m[1];
      }
    }
    foreach ($pids as $pid) {
      posix_kill($pid, 2);
    }
    // Terminate the main child process.
    proc_terminate($process);
  }
  proc_close($process);
}
