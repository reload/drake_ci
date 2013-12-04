<?php

$tasks['ci-run-behat'] = array(
  'action' => 'run-behat',
  'root' => context('[@self:site:root]'),
  'baseline-package' => context_optional('baseline-package'),
  'db-su' => context('db-su'),
  'db-su-pw' => context_optional('db-su-pw'),
  'php' => context_optional('php'),
  'selenium-wd-host' => context_optional('selenium-wd-host'),
  // TODO: Get this from the main site?
  'profile' => context_optional('profile'),
  'site-port' => context_optional('site-port'),
  'site-host' => context_optional('site-host'),
  'test-port' => context_optional('test-port'),
  'test-host' => context_optional('test-host'),
  'output-dir' => context_optional('output-dir', context('[@self:site:root]/tests/behat')),
  'behat-features' => context_optional('behat-features'),
  'behat-config' => context_optional('behat-config'),
  'behat-dir' => context_optional('behat-dir'),
  'generate-sites-php' => context_optional('generate-sites-php'),
);

// See http://saucelabs.com/docs/additional-config#desired-capabilities
$behat_capabillities = array('platform', 'browser', 'version', 'name', 'name',
                             'build', 'tags', 'passed', 'custom-data', 'record-video',
                             'video-upload-on-pass', 'record-screenshots',
                             'capture-html', 'webdriver.remote.quietExceptions',
                             'sauce-advisor', 'selenium-version', 'single-window',
                             'user-extensions-url', 'firefox-profile-url',
                             'max-duration', 'command-timeout', 'idle-timeout',
                             'prerun', 'tunnel-identifier', 'screen-resolution',
                             'disable-popup-handler', 'avoid-proxy', 'public');
// Would be nice just to be able to handle this dynamically, but for now this
// will have to do.
foreach ($behat_capabillities as $cap) {
  $key = SELENIUM_CAP_PREFIX . $cap;
  $tasks['ci-run-behat'][$key] = context_optional($key);
}

/**
 * Simpletest action. Runs simpletests from the specified files.
 */
$actions['run-behat'] = array(
  'default_message' => 'Behat.',
  'callback' => 'drake_ci_behat_test',
  'parameters' => array(
    'output-dir' => 'Directory to output to',
    'behat-features' => array(
      'description' => 'Behat features to execute, relative to behat-dir. Defaults to "features/"',
      'default' => 'features/',
    ),
    'behat-config' => array(
      'description' => 'Behat configuration file, relative to behat-dir, defaults to config/behat.yml',
      'default' => 'config/behat.yml',
    ),
    'behat-dir' => array(
      'description' => 'Behat directory relative to the drupal root, defaults to "sites/all/tests/behat" or "profiles/<profile>/tests/behat" if profile is specified',
      'default' => NULL,
    ),
    'db-su' => 'Database Super-user allowed to create databases',
    'db-su-pw' => array(
      'description' => 'Password for the database Superuser',
      'default' => NULL,
    ),
    'php' => array(
      'description' => 'full path to the php-executable to use, defaults to php (ie. first php on path)',
      'default' => 'php',
    ),
    'selenium-wd-host' => array(
      'description' => 'Webdriver host',
      'default' => NULL,
    ),

    'baseline-package' => array(
      'description' => 'Baseline package - an aegir backup',
      'default' => NULL,
    ),
    'capabilities' => array(
      'description' => 'Remote Webdriver desired capabilities, see http://saucelabs.com/docs/additional-config#desired-capabilities if set this value will override any ' . SELENIUM_CAP_PREFIX . "-* parameters",
      'default' => NULL,
    ),
    'site-port' => array(
      'description' => 'Port to use for the temporary site, default is to pick a random port',
      'default' => NULL,
    ),
    'site-host' => array(
      'description' => 'Hostname of the site, should be accessible to saucelabs',
      'default' => 'localhost',
    ),
    'test-port' => array(
      'description' => 'Use if the public port as seen from saucelabs is different from the local port specified via site-port',
      'default' => NULL,
    ),
    'test-host' => array(
      'description' => 'Use if the public hostname as seen from saucelab is different from the hostname specified via site-host',
      'default' => NULL,
    ),
    'profile' => array(
      'description' => 'Profile to use if the site is to be installed, also used for naming the temporary database.',
      'default' => NULL,
    ),
    'no-cleanup' => array(
      'description' => 'Whether to delete temporary site and database used for the test after execution has been completed.',
      'default' => FALSE,
    ),
    'max-executiontime' => array(
      'description' => 'Maximum number of seconds we should wait for Behat to execute',
      'default' => 60 * 60,
    ),
    'generate-sites-php' => array(
      'description' => 'Generate a sites.php if site-host and test-host differs',
      'default' => FALSE,
    ),
    // TODO: Allow modules to be enabled/disabled prior to execution.
  ),
);

// Mirror the settings from the task. A bit of a hack, but in lack of a way to
// inspect the task arguments to proviede dynamic parameters this is the next
// best thing.
foreach ($tasks['ci-run-behat'] as $key) {
  if (strpos($key, SELENIUM_CAP_PREFIX) === 0) {
    $name =  substr($key, strlen(SELENIUM_CAP_PREFIX));
    $actions['run-behat']['parameters'][$key] = array(
      'description' => 'Selenium desired capabillity "' . $name . '", used if "capabilities" is not specified.',
      'default' => NULL,
    );
  }
}

/**
 * Boot up a site based on a baseline package and test it via saucelabs.
 */
function drake_ci_behat_test($context) {
  // Prepare vars from context.

  // If port was not specified, pick a random port. With so many to choose from,
  // we're unlikely to collide.
  $port = !empty($context['site-port']) ? $context['site-port'] : mt_rand(50000, 60000);
  $site_local_uri = $context['site-host'] . ':' . $port;

  $cleanup = !((bool) $context['no-cleanup']);
  $site_dir = $port . '.' . $context['site-host'];
  $profile = empty($context['profile']) ? 'default' : $context['profile'];
  $target_site_path = $context['root'] . '/sites/' . $site_dir;

  // Make output-dir absolute.
  if (strpos($context['output-dir'], '/') !== 0) {
    $output_dir = $context['root'] . '/' . $context['output-dir'];
  }
  else {
    $output_dir = $context['output-dir'];
  }

  // Create the output_dir if it does not exist.
  if (!file_exists($output_dir) && !mkdir($output_dir)) {
    return drake_action_error(dt('Output dir "%output_dir" could not be found and could not be created.', array('%output_dir' => $output_dir)));
  }

  // Prepare an absolute behat-dir and check it exists.
  if (empty($context['behat-dir'])) {
    // Generate a behatdir.
    if (!empty($context['profile'])) {
      $context['behat-dir'] = 'profiles/' . $context['profile'] . '/tests/behat';
    }
    else {
      $context['behat-dir'] = 'sites/all/tests/behat';
    }
  }

  // Make sure the behat-dir is absolute and have no trailing slash.
  if (strpos($context['behat-dir'], '/') !== 0) {
    // Make the relative path absolute.
    $behat_dir = $context['root'] . '/' . $context['behat-dir'];
  }
  else {
    // Already absolute, nothing to do.
    $behat_dir = $context['behat-dir'];
  }
  $behat_dir = rtrim($behat_dir, '/');
  if (!is_dir($behat_dir) or !is_readable($behat_dir)) {
    // "Soft", directory might just be missing on this branch.
    drush_log(dt('Could not access behat-dir %behat_dir, skipping test-run', array('%behat_dir' => $behat_dir)), 'warning');
    return TRUE;
  }

  // Contexts processed, carry on with the actual work.
  // If the site dir exists, move it out of the way.
  if (file_exists($target_site_path)) {
    $new_name = $context['root'] . '/sites/' . $site_dir . '_archived_' . time();;
    if (!rename($target_site_path, $new_name)) {
      return drake_action_error(dt('Site_dir %site_dir already exists and could not rename it, exiting.', array('%site_dir' => $site_dir)));
    }
  }

  // Check output directory.
  if (!is_dir($output_dir) && !mkdir($output_dir, 0777, TRUE)) {
    return drake_action_error(dt('Could not access or create output-dir "%dir"', array('%dir', $output_dir)));
  }

  // Prepare the sitedir, cd to it so that we can unpack the baseline package
  // and start the php webserver.
  drush_mkdir($target_site_path);
  // Register dir/files for deletion when we're done.
  if ($cleanup) {
    // Delete site dir.
    drush_register_file_for_deletion($target_site_path, TRUE);
  }

  // Prepare a sites.php if site-host is set.
  if (!empty($context['test-host']) && $context['test-host'] != $context['site-host']) {
    if (!$context['generate-sites-php']) {
      // Entry in sites.php needed by auto-generation is disabled (default).
      // Issue a warning and continue.
      drush_log("test-host and site-host differs, please make sure sites.php maps the external hostname to the local site or enable automatic sites.php generation via generate-sites-php", 'warning');
    }
    else {
      // User has requested a sites.php to be generated automatically, generate
      // one while preserving any existing sites.php by moving it out of the way
      // temporarily.
      $sites_php = $context['root'] . '/sites/' . 'sites.php';
      if (file_exists($sites_php)) {
        if (!rename($sites_php, $sites_php . "_org")) {
          return drake_action_error(dt('Could not rename %file.', array('%file', $sites_php)));
        }
      }

      // Make sure the original file is restored after we are done.
      register_shutdown_function(function () use ($sites_php) {
        unlink($sites_php);
        rename($sites_php . "_org", $sites_php);
      });

      // Write our own sites.php truncating any existing file.
      $fh = fopen($sites_php, 'w+');
      if (!$fh) {
        return drake_action_error(dt('Could not write to %file, the file needs to be updated as test-host differs from site-host.', array('%file', $sites_php)));
      }
      $buffer = "<?php\n";
      $buffer .= "\$sites['" . $context['test-host'] . "'] = '" . $site_dir . "';\n";
      fwrite($fh, $buffer);
      fclose($fh);
    }
  }

  if (file_exists($context['baseline-package'])) {
    // Unpack the baseline package into the specified target site-path.
    if (!drake_ci_behat_extract_baseline($site_dir, $target_site_path, $context)) {
      return FALSE;
    }
  }

  $db_spec = array(
    'driver' => 'mysql',
    'database' => (strlen($profile) > 8 ? substr($profile, 0, 8) : $profile) . "_" . $port,
    'host' => 'localhost',
    'username' => $context['db-su'],
    'password' => $context['db-su-pw'],
    'port' => 3306,
    'prefix' => '',
    'collation' => 'utf8_general_ci',
  );
  // Create the database, this requires the db-su user to have a CREATE DATABASE
  // grant.
  if (!_drush_sql_create($db_spec)) {
    return drake_action_error(dt('Could not create database %database.', array('%database', $db_spec['database'])));
  }
  else {
    drush_log(dt('Created temporary database %dbname', array('%dbname' => $db_spec['database'])));
  }

  // Make sure the database gets dropped at exit.
  if ($cleanup) {
    register_shutdown_function('_drake_ci_drop_db_shutdown', $db_spec);
  }

  // Setup settings.php
  $settings_path = $target_site_path . '/settings.php';

  // First, handle an existing settings.php. Use it to detect an aegir backup
  // and set some of our own settings. Later on we're going to get rid of any
  // packaged settings.php.
  if (file_exists($settings_path)) {
    $baseline_settings = file_get_contents($settings_path);

    // Aegir.
    if (strpos($baseline_settings, 'aegir_api') !== FALSE) {
      // We now know the location of the files.
      $site_settings['conf']['file_public_path'] = 'sites/' . $site_dir . '/files';
      $site_settings['conf']['file_temporary_path'] = 'sites/' . $site_dir . '/private/temp';
      $site_settings['conf']['file_private_path'] = 'sites/' . $site_dir . '/private/files';
    }
    else {
      // A bit harsh, but it will have to do for now.
      $site_settings['conf']['file_public_path'] = 'sites/' . $site_dir . '/files';
      $site_settings['conf']['file_temporary_path'] = '/tmp';
      $site_settings['conf']['file_private_path'] = 'sites/' . $site_dir . '/files';
    }

    // Fix permissions as we're going to overwrite settings.php later.
    if (!chmod($settings_path, 0755)) {
      return drake_action_error(dt('Could make %file writable', array('%file', $settings_path)));
    }
  }

  $site_settings['databases']['default']['default'] = $db_spec;

  // Write our own settings.php, truncating any existing file.
  $fh = fopen($settings_path, 'w+') or die("can't open $settings_path");
  $buffer = "<?php\n";
  foreach ($site_settings as $setting => $value) {
    $buffer .= "\$$setting = " . var_export($value, TRUE) . ";\n";
  }
  fwrite($fh, $buffer);
  fclose($fh);

  // Populate the database by either importing a dump or site-installing.
  $sqldump_path = $target_site_path . '/database.sql';
  $drush_invoke_options['root'] = $context['root'];
  $drush_invoke_options['uri'] = $site_local_uri;

  if (file_exists($sqldump_path)) {
    // Database-dump found.
    drush_log(dt('Importing database.sql into the database %dbname', array('%dbname' => $db_spec['database'])), 'ok');
    $success = _drush_sql_query(NULL, $db_spec, $sqldump_path);
    // Import database.
    if (!$success) {
      return drake_action_error(dt('Could not import database-dump from %dump.', array('%dump', $target_site_path . '/database.sql')));
    }

    // Clear cache as the imported database might contains some paths that needs
    // updating.
    drush_log('Flushing site-cache after database-import', 'ok');
    $res = drush_invoke_process(NULL, 'cache-clear', array('all'), $drush_invoke_options, TRUE);

    if (!$res || $res['error_status'] != 0) {
      return drake_action_error(dt('Error clearing cache.'));
    }

  }
  else {
    // Site install.
    $args = array($profile);
    $db_url = $db_spec['driver'] . '://' .  $db_spec['username'] . ':' . $db_spec['password'] . '@' . $db_spec['host'] . '/' . $db_spec['database'];
    $drush_invoke_options['db-url'] = $db_url;
    $drush_invoke_options['sites-subdir'] = $site_dir;

    drush_log(dt('Installing site %sitedir with profile %profile', array('%sitedir' => $site_dir, '%profile' => $profile)), 'ok');
    $res = drush_invoke_process(NULL, 'site-install', $args, $drush_invoke_options, TRUE);

    if (!$res || $res['error_status'] != 0) {
      return drake_action_error(dt('Error installing site.'));
    }
  }

  if (!drake_ci_behat_start_webserver($site_local_uri, $port, $context)) {
    return FALSE;
  }


  // Use a temporary log file, to avoid buffers being filled.
  $stdout_logfile = $output_dir . '/behat-saucelabs-' . $port . '-' . posix_getpid() . '.log';
  $errout_logfile = $output_dir . '/behat-saucelabs-error-' . $port . '-' . posix_getpid() . '.log';
  drush_register_file_for_deletion($log_file);
  $descriptorspec = array(
    0 => array('file', '/dev/null', 'r'),
    1 => array('file', $stdout_logfile, 'w'),
    2 => array('file', $errout_logfile, 'w'),
  );

  $mink_extension_params = array (
    'base_url' => 'http://' . (!empty($context['test-host']) ? $context['test-host'] : $context['site-host']) . ':' . (!empty($context['test-port']) ? $context['test-port'] : $port),
  );

  if (!empty($context['selenium-wd-host'])) {
    $mink_extension_params['selenium2']['wd_host'] = $context['selenium-wd-host'];
  }

  if (!empty($context['capabilities'])) {
    $mink_extension_params['selenium2']['capabilities'] = $context['capabilities'];
  }
  else {
    // Extract capabillities dynamically.
    // Generate name if not specified.
    $caps = array();
    if (empty($context['selenium-cap-name'])) {
      // TODO: use timestamp instead.
      $context['selenium-cap-name'] = $context['site-host'] . '-' . $context['profile'] . '-' . date("YmdHis");
    }
    foreach ($context as $key => $value) {
      if (strpos($key, SELENIUM_CAP_PREFIX) === 0 && !empty($value)) {
        $caps[] = "'" . substr($key, strlen(SELENIUM_CAP_PREFIX)) . "':'" . $value . "'";
      }
    }

    // Wrap in brances seperate by comma.
    if (count($caps) > 0) {
      $mink_extension_params['selenium2']['capabilities'] = '{' . implode(',', $caps) . '}';
    }
  }

  $behat_config = $context['behat-config'];

  // TODO: use a YAML parser.
  // Inject the video-url logger extension into behat-config.
  $behat_config_full = $behat_dir . '/' . $behat_config;
  if (!file_exists($behat_config_full) || !is_writable($behat_config_full)) {
    return drake_action_error(dt('Could not update behat-config %file, either file could not be found or written to.', array('%file' => $behat_config_full)));
  }
  copy($behat_config_full, $behat_config_content . "_org");
  $behat_config_content = file_get_contents($behat_config_full);
  // Make sure the extension is not already in there.
  if (!preg_match('/log_videourl.php/m', $behat_config_content)) {
    // A bit crude, but should work for now.
    $behat_extension_path = "'" . dirname(__FILE__) . "/../behat_extensions/log_videourl.php': ~";
    // Find the extesions: entry, inject an indented entry after it.
    $behat_config_content = preg_replace('/(\s+)(extensions:).*$/m', "\$1\$2\$1  " . $behat_extension_path, $behat_config_content);

    // Preserve the original file.
    copy($behat_config_full, $behat_config_full . '_org');
    file_put_contents($behat_config_full, $behat_config_content);

    // Make sure we move the original file back.
    register_shutdown_function(function() use ($behat_config_full){
      rename($behat_config_full . '.org', $behat_config_full);
    });
  }
  $behat_features = $context['behat-features'];

  $behat_proc_env = $_ENV;
  $behat_proc_env['MINK_EXTENSION_PARAMS'] = http_build_query($mink_extension_params);

  // Prepare a string-version for debugging.
  $behat_proc_env_pretty = array();
  foreach ($behat_proc_env as $key => $value) {
    $behat_proc_env_pretty[] = $key . '=' . $value;
  }
  $behat_proc_env_pretty = implode(':', $behat_proc_env_pretty);

  // The HTML and Junit formatter needs a file and dir respectivly.
  $out_html = $output_dir . '/behat-result.html';
  $out_junit = $output_dir;

  $cmd = 'behat -v -c ' . escapeshellarg($behat_config) . ' -f pretty,junit,html --out ,' . escapeshellarg($out_junit) . ',' . escapeshellarg($out_html) . ' ' . escapeshellarg($behat_features);
  drush_log(dt('Running ' . $cmd . ' in behat-dir: %dir with environment %env', array('%dir' => $behat_dir, '%env' => $behat_proc_env_pretty)), 'debug');
  drush_log(dt('Starting behat session named %session', array('%session' => $context['selenium-cap-name'])), 'ok');
  $behat_process = proc_open($cmd, $descriptorspec, $pipes, $behat_dir, $behat_proc_env);
  register_shutdown_function('_drake_ci_kill_process_shutdown', $behat_process);

  $max_executiontime = $context['max-executiontime'];
  $start = time();

  // Wait a sec.
  sleep(1);
  // Then get the process status.
  $proc_status = proc_get_status($behat_process);
  $force_exit = FALSE;
  if ($behat_process && $proc_status['running']) {
    $procs_to_be_cleaned[] = $behat_process;

    // Wait for the process to stop.
    do {
      sleep(1);

      // TODO: Clean up behat process.
      $proc_status = proc_get_status($behat_process);
      // Halt if max execution-time has passed.
      $force_exit = time() > ($start + ($max_executiontime));
    } while($proc_status['running'] && $proc_status['pid'] && !$force_exit);

    // Scan the output logfile for video-links, also dump to log.
    // TODO: we only do this to get the progress, would be nicer to just allow
    // it to go to stdout directly.
    $stdout_content = file($stdout_logfile);
    foreach ($stdout_content as $line) {
      if (preg_match('#saucelabs.com/jobs#', $line)) {
        $matches[] = $line;
      }
      else {
        drush_log(rtrim($line), 'success');
      }
    }

    // Log the video-url via drush, and generate a html-report.
    // TODO: Inject the URL into the behat html-report instead.
    drush_log("Video URL:", 'success');
    $matches = array_unique($matches);
    foreach ($matches as $match) {
      drush_log($match, 'success');
      $matches_markup .= "<li><a href=\"$match\">$match</a>\n";
    }
    $report = <<<EOT
<html><head></head><body>
<h1>Saucelabs results</h1>
<ul>
  $matches_markup
</ul>
</body></html>
EOT;
    file_put_contents($output_dir . '/video_url.html', $report);
  }
  else {
    if (!$behat_process) {
      return drake_action_error(dt('Execute %cmd.', array('%cmd' => $cmd)));
    }
  }
  if ($force_exit) {
    return drake_action_error(dt('Gave up waiting for behat to complete, more than %max second passed.', array('%max' => $max_executiontime)));
  }

  drush_log(dt('Behat execution completed in %sec seconds, output can be found in %outputdir', array('%sec' => (time() - $start), '%outputdir' => $output_dir)), 'ok');
  // Check status and finish up.
  if ($proc_status['exitcode'] !== 0) {
    drush_log("Behat error output", 'error');
    $errorout_lines = file($errout_logfile);
    foreach ($errorout_lines as $line) {
      drush_log(rtrim($line), 'error');
    }

    return drake_action_error(dt('Non-zero exit-code(%exit) from behat indicates an error during execution, marking test as failed', array('%exit' => $proc_status['exitcode'])));
  }
  else {
    drush_log('Test completed successfully', 'success');
    return TRUE;
  }
}

/**
 * @param $site_local_uri
 * @param $port
 * @param $context
 * @return bool
 */
function drake_ci_behat_start_webserver($site_local_uri, $port, $context) {
// Get a webserver up and running.

  // Use a temporary log file, to avoid buffers being filled.
  $log_file = '/tmp/behat-webserver' . $port . '-' . posix_getpid() . '.log';
  drush_register_file_for_deletion($log_file);
  $descriptorspec = array(
    0 => array('file', '/dev/null', 'r'),
    1 => array('file', '/dev/null', 'w'),
    2 => array('file', $log_file, 'w'),
  );

  // We'd like to use drush runserver instead, but in initial testing runserver
  // would cause core tests to fail on login, while this would not.

  $cmd = $context['php'] . ' -t ' . $context['root'] . ' -S ' . $site_local_uri . ' ' . dirname(dirname(__FILE__)) . '/router.php';
  drush_log("Executing command " . $cmd, 'debug');
  $php_process = proc_open($cmd, $descriptorspec, $pipes, $context['root']);
  register_shutdown_function('_drake_ci_kill_process_shutdown', $php_process);

  // Wait a sec.
  sleep(1);
  // Then check that the server started.
  $proc_status = proc_get_status($php_process);
  if (!$php_process || !$proc_status['running']) {
    return drake_action_error(dt('Could not start internal web server.'));
  }

  drush_log(dt('Webserver running at http://%host:%port %proc',
    array(
      '%host' => $context['drupal-multisite-host'],
      '%port' => $port,
      '%proc' => $php_process
    )
  ), 'ok');
  return TRUE;
}

/**
 * @param $profile
 * @param $db_spec
 * @param $site_dir
 * @param $drush_invoke_options
 * @return bool
 */
function drake_ci_behat_site_install($profile, $db_spec, $site_dir, $drush_invoke_options) {
// Site install.
  $args = array($profile);
  $db_url = $db_spec['driver'] . '://' . $db_spec['username'] . ':' . $db_spec['password'] . '@' . $db_spec['host'] . '/' . $db_spec['database'];
  $drush_invoke_options['db-url'] = $db_url;
  $drush_invoke_options['sites-subdir'] = $site_dir;

  drush_log(dt('Installing site %sitedir with profile %profile', array(
    '%sitedir' => $site_dir,
    '%profile' => $profile
  )), 'ok');
  $res = drush_invoke_process(NULL, 'site-install', $args, $drush_invoke_options, TRUE);

  if (!$res || $res['error_status'] != 0) {
    return drake_action_error(dt('Error installing site.'));
  }
  else {
    return TRUE;
  }
}
/**
 * @param $site_dir
 * @param $target_site_path
 * @param $context
 * @return bool
 */
function drake_ci_behat_extract_baseline($site_dir, $target_site_path, $context) {
// Cd to the site-directory to untar the baseline package.
  $oldcwd = getcwd();
  chdir($target_site_path);

  // Unpack baseline package.
  // Method taken from backup.provision.inc from aegirs provision.
  $command = 'gunzip -c %s | tar pxf -';
  drush_log(dt('Running: %command in %target',
    array(
      '%command' => sprintf($command, $context['baseline-package']),
      '%target'  => $target_site_path,
    )
  ));
  $pathinfo = pathinfo($context['baseline-package']);
  drush_log(dt('Unpacking baseline package %package into %target.', array(
    '%package' => $pathinfo['filename'],
    '%target'  => $site_dir
  )), 'ok');

  $result = drush_shell_exec($command, $context['baseline-package']);

  // Done, go back to original dir.
  chdir($oldcwd);

  if (!$result) {
    return drake_action_error(dt('Could not unpack baseline package %package into the temporary site-dir %target.', array(
      '%package' => $context['baseline-package'],
      '%target'  => $target_site_path
    )));
  }
  else {
    return TRUE;
  }
}
 * Shutdown function drop a database.
 */
function _drake_ci_drop_db_shutdown($db_spec) {
  // Drop the database.
  if ($db_spec['driver'] == 'mysql') {
    $dbname = '`' . $db_spec['database'] . '`';
    $sql = sprintf('DROP DATABASE IF EXISTS %s;', $dbname);

    // Strip the database-name out of the spec as it does not exist yet.
    $drop_spec = $db_spec;
    unset($drop_spec['database']);
    $success = _drush_sql_query($sql, $drop_spec);
    if ($success) {
      drush_log(dt('Database %dbname successfully dropped', array('%dbname', $db_spec['database'])));
    }
    else {
      drush_log(dt('Database %dbname successfully dropped', array('%dbname', $db_spec['database'])), 'error');
    }
  }
  else {
    drush_log(dt('Could not drop database, unsupported driver "%driver"', array('%driver' => $db_spec['driver'])));
  }
}
