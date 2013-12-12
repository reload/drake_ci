<?php

/**
 * @file
 * Generic code checking/testing and analysis tasks.
 */

$api = 1;

/**
 * Prefix used for Selenium Desired Capability parameters.
 */
define('SELENIUM_CAP_PREFIX', 'selenium-cap-');

/*
 * Default context for tasks.
 */
$context = array(
  'root' => context('@self:site:root'),
);

/*
 * Filesets for the tasks.
 */
$filesets['php'] = array(
  'include' => array(
    '*.php',
    '*.module',
    '*.install',
    '*.inc',
    '*.profile',
    '*.test',
  ),
);

$filesets['php-generated'] = array(
  'include' => array(
    '**/*.features.*',
    '**/*.feeds_importer_default.inc',
    '**/*.field_group.inc',
    '**/*.layouts.inc',
    '**/*.pages_default.inc',
    '**/*.panels_default.inc',
    '**/*.panelizer.inc',
    '**/*.strongarm.inc',
    '**/*.views_default.inc',
  ),
);

$filesets['contrib'] = array(
  'include' => array(
    '**/contrib/**',
    '**/development/**',
    '**/libraries/**',
  ),
);

$filesets['core'] = array(
  // These patterns are anchored at /.
  'include' => array(
    // Matches files in the Drupal root dir.
    '/*',
    '/includes/**',
    '/misc/**',
    '/scripts/**',
    '/modules/**',
    '/themes/**',
    '/sites/*',
    '/sites/default/**',
    '/sites/*/settings.php',
    '/profiles/minimal/**',
    '/profiles/standard/**',
    '/profiles/testing/**',
    '/profiles/default/**',
  ),
);

$filesets['js'] = array(
  'include' => array('*.js'),
  // Minimized JavaScript files should not be analyzed.  In their optimized
  // state they can not be expected to conform to coding standards.
  'exclude' => array('*.min.js'),
);

$filesets['css'] = array(
  'include' => array('*.css'),
);

/*
 * Convinience filesets.
 *
 * Default commands use these, but they can be extended or redifined as the user
 * sees fit.
 */
$filesets['php-custom'] = array(
  'dir' => context('root'),
  'extend' => array(
    'php',
    'no-php-generated',
    'no-core',
    'no-contrib',
  ),
);

$filesets['js-custom'] = array(
  'dir' => context('root'),
  'extend' => array(
    'js',
    'no-core',
    'no-contrib',
  ),
);

$filesets['css-custom'] = array(
  'dir' => context('root'),
  'extend' => array(
    'css',
    'no-core',
    'no-contrib',
  ),
);

$filesets['all-custom'] = array(
  'dir' => context('root'),
  'extend' => array(
    'php',
    'no-php-generated',
    'css',
    'js',
    'no-core',
    'no-contrib',
  ),
);

/**
 * Fileset that contains everything.
 */
$filesets['all'] = array(
  'dir' => context('root'),
);

/*
 * Convinience tasks.
 *
 * Default implementation of all actions so that site drakefiles doesn't have to
 * implement them. They can of course be overwritten.
 */
$tasks['ci-clean'] = array(
  'action' => 'ci-clean',
  'output-dir' => context_optional('output-dir'),
);

$tasks['check-all'] = array(
  'depends' => array(
    'check-php',
    'check-js',
    'check-css',
    'run-tests',
  ),
);

$tasks['check-php'] = array(
  'depends' => array(
    'ci-clean',
    'php-lint',
    'php-debug',
    'php-cs',
    'php-md',
    'php-cpd',
    'php-loc',
  ),
);

$tasks['check-js'] = array(
  'depends' => array(
    'ci-clean',
    'js-hint',
    'js-debug',
  ),
);

$tasks['check-css'] = array(
  'depends' => array(
    'ci-clean',
    'css-lint',
  ),
);

$tasks['run-tests'] = array(
  // Might seem overkill with a task with only one dependency, but
  // behat/selenium/other tests could be added.
  'depends' => array(
    'run-simpletests',
  ),
);


require_once dirname(__FILE__) . '/tasks/php-lint.inc';
require_once dirname(__FILE__) . '/tasks/php-debug.inc';
require_once dirname(__FILE__) . '/tasks/php-md.inc';
require_once dirname(__FILE__) . '/tasks/php-cpd.inc';
require_once dirname(__FILE__) . '/tasks/php-cs.inc';
require_once dirname(__FILE__) . '/tasks/php-loc.inc';

require_once dirname(__FILE__) . '/tasks/js-hint.inc';
require_once dirname(__FILE__) . '/tasks/js-debug.inc';

require_once dirname(__FILE__) . '/tasks/css-lint.inc';

require_once dirname(__FILE__) . '/tasks/run-simpletests.inc';

require_once dirname(__FILE__) . '/tasks/package-zip.inc';

require_once dirname(__FILE__) . '/tasks/run-behat.inc';
require_once dirname(__FILE__) . '/tasks/baseline.inc';

/*
 * Clean up output directory, if specified.
 */
$actions['ci-clean'] = array(
  'default_message' => 'Setting up/cleaning output directories.',
  'callback' => 'drake_ci_clean',
  'parameters' => array(
    'output-dir' => array(
      'description' => 'Directory for results of analysis.',
      'default' => '',
    ),
  ),
);



/**
 * CI clean action. Empty the output directory, if specified.
 */
function drake_ci_clean($context) {
  if (!empty($context['output-dir'])) {
    if (!file_exists(dirname($context['output-dir']))) {
      return drake_action_error(dt('Parent dir to output-dir does not exist.'));

    }
    if (file_exists($context['output-dir'])) {
      drush_delete_dir($context['output-dir']);
    }
    mkdir($context['output-dir']);
    if (!is_writable($context['output-dir'])) {
      return drake_action_error(dt('Error setting up output-dir.'));
    }
  }
}

/*
 * Helper functions for action callbacks.
 */

/**
 * Shutdown function to end the PHP server process.
 */
function _drake_ci_kill_process_shutdown($process) {
  drush_log("Cleaning up processes after shutdown");
  // We assume that all is dandy if the server is still running. Can't count on
  // return code from proc_close.
  $proc_status = proc_get_status($process);
  $php_rc = $proc_status["running"] ? 0 : $proc_status["exitcode"];
  if ($php_rc != 0) {
    drush_set_error('PHP_SERVER_ERROR', dt("The PHP server process returned error."));
  }

  if ($proc_status['running'] && $proc_status['pid']) {
    drush_log(dt('Process started by command "%cmd" is still runnning, killing...', array('%cmd' => $proc_status['command'])));
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
      drush_log("Killing process with pid " . $pid);
      posix_kill($pid, 2);
    }
    // Terminate the main child process.
    proc_terminate($process);
  }
  proc_close($process);
}

/**
 * Execute a command that might use a non-zero exit code.
 *
 * This works much like drush_shell_exec(), but only returns FALSE if the
 * command couldn't be run. If the command returns a non-zero exit code, this
 * will still return TRUE, unlike drush_shell_exec().
 *
 * The return code of the command is saved to the SHELL_RC_CODE context for
 * inspection.
 */
function drake_ci_shell_exec($cmd) {
  $args = func_get_args();
  // Do not change the command itself, just the parameters.
  for ($x = 1; $x < count($args); $x++) {
    $args[$x] = drush_escapeshellarg($args[$x]);
  }
  // Important: we allow $args to take one of two forms here.  If
  // there is only one item in the array, it is the already-escaped
  // command string, but otherwise sprintf is used.  In the case
  // of pre-escaped strings, sprintf will fail if any of the escaped
  // parameters contain '%', so we must not call sprintf unless necessary.
  if (count($args) == 1) {
    $command = $args[0];
  }
  else {
    $command = call_user_func_array('sprintf', $args);
  }

  if (drush_get_context('DRUSH_VERBOSE') || drush_get_context('DRUSH_SIMULATE')) {
    drush_print('Executing: ' . $command, 0, STDERR);
  }
  if (!drush_get_context('DRUSH_SIMULATE')) {
    exec($command . ' 2>&1', $output, $result);
    _drush_shell_exec_output_set($output);

    if (drush_get_context('DRUSH_DEBUG')) {
      foreach ($output as $line) {
        drush_print($line, 2);
      }
    }

    // The sh command returns 127 when it couldn't find the command.
    if ($result == 127) {
      $tmp = explode(' ', $cmd);
      return drake_action_error(dt('Error running @command: @message', array(
            '@message' => implode("\n", drush_shell_exec_output()),
            '@command' => trim($tmp[0]))));
    }
    // Save the return code in context.
    drush_set_context('SHELL_RC_CODE', $result);
    // Return true.
    return TRUE;
  }
  else {
    return TRUE;
  }
}

/**
 * Flatten a path to a file name.
 *
 * Flattens path/to/file-name.php to path-to-file--name.php
 */
function drake_ci_flatten_path($path) {
  return strtr($path, array('-' => '--', '/' => '-'));
}
