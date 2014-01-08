<?php
use Symfony\Component\Yaml\Exception\ParseException;
use Symfony\Component\Yaml\Parser;
use Symfony\Component\Yaml\Dumper;
require dirname(__FILE__) . '/../vendor/autoload.php';

$tasks['ci-run-behat'] = array(
  'action' => 'run-behat',
  'root' => context('[@self:site:root]'),
  'baseline-package' => context_optional('baseline-package'),
  'baseline-type' => context_optional('baseline-type'),
  'db-su' => context('db-su'),
  'db-su-pw' => context_optional('db-su-pw'),
  'php' => context_optional('php'),
  'phantomjs-path' => context_optional('phantomjs-path'),
  'selenium-wd-host' => context_optional('selenium-wd-host'),
  // TODO: Get this from the main site?
  'profile' => context_optional('profile'),
  'behat-profile' => context_optional('behat-profile'),
  'drupal-multisite-port' => context_optional('drupal-multisite-port'),
  'drupal-multisite-host' => context_optional('drupal-multisite-host'),
  'external-site-port' => context_optional('external-site-port'),
  'external-site-host' => context_optional('external-site-host'),
  'output-dir' => context_optional('output-dir', '/tmp/run-behat-output'),
  'behat-features' => context_optional('behat-features'),
  'behat-config' => context_optional('behat-config'),
  'behat-dir' => context_optional('behat-dir'),
  'behat-strict' => context_optional('behat-strict'),
  'no-generate-sites-php' => context_optional('no-generate-sites-php'),
  'no-behat' => context_optional('no-behat'),
  'no-behat-log' => context_optional('no-behat-log'),
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
    'behat-strict' => array(
      'description' => 'Whether to run behat in strict mode - defaults to TRUE',
      'default' => TRUE,
    ),
    'behat-profile' => array(
      'description' => 'Behat test-profile to use, "default" if unspecified.',
      'default' => 'default',
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
    'phantomjs-path' => array(
      'description' => 'full path to the phantomjs-executable to use if the browser is phantomjs',
      'default' => NULL,
    ),
    'selenium-wd-host' => array(
      'description' => 'Webdriver host',
      'default' => NULL,
    ),

    'baseline-package' => array(
      'description' => 'Baseline package - an aegir 1 backup or an archive-dump with a single site',
      'default' => NULL,
    ),
    'baseline-type' => array(
      'description' => 'aegir or archive-dump (Default)',
      'default' => 'archive-dump',
    ),
    'capabilities' => array(
      'description' => 'Remote Webdriver desired capabilities, see http://saucelabs.com/docs/additional-config#desired-capabilities if set this value will override any ' . SELENIUM_CAP_PREFIX . "-* parameters",
      'default' => NULL,
    ),
    'drupal-multisite-port' => array(
      'description' => 'Port to use for the temporary site, default is to pick a random port',
      'default' => NULL,
    ),
    'drupal-multisite-host' => array(
      'description' => 'Hostname of the site, should be accessible to saucelabs',
      'default' => 'localhost',
    ),
    'external-site-port' => array(
      'description' => 'Use if the public port as seen from saucelabs is different from the local port specified via drupal-multisite-port',
      'default' => NULL,
    ),
    'external-site-host' => array(
      'description' => 'Use if the public hostname as seen from saucelab is different from the hostname specified via drupal-multisite-host',
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
    'no-generate-sites-php' => array(
      'description' => 'Do not generate a sites.php if drupal-multisite-host and external-site-host differs',
      'default' => FALSE,
    ),
    'no-behat' => array(
      'description' => 'Just get the site up and running, but dont start behat, instead wait for the user to request shutdown.',
      'default' => FALSE,
    ),
    'no-log' => array(
      'description' => 'Dump behat output to standard-out instead of a logfil',
      'default' => FALSE,
    ),
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
  $port = !empty($context['drupal-multisite-port']) ? $context['drupal-multisite-port'] : mt_rand(50000, 60000);

  $site_local_uri = $context['drupal-multisite-host'] . ':' . $port;

  $cleanup = !((bool) $context['no-cleanup']);
  $profile = empty($context['profile']) ? 'default' : $context['profile'];

  $behat_profiles = explode(',', $context['behat-profile']);

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

  // Check output directory.
  if (!is_dir($output_dir) && !mkdir($output_dir, 0777, TRUE)) {
    return drake_action_error(dt('Could not access or create output-dir "%dir"', array('%dir', $output_dir)));
  }


  // Contexts processed, carry on with the actual work.

  // TODO Extract the entire "setup a site-dir" into a function / task.
  $site_dir = $port . '.' . $context['drupal-multisite-host'];
  $target_site_path = $context['root'] . '/sites/' . $site_dir;

  // If the site dir exists, move it out of the way.
  if (file_exists($target_site_path)) {
    $new_name = $context['root'] . '/sites/' . $site_dir . '_archived_' . time();;
    if (!rename($target_site_path, $new_name)) {
      return drake_action_error(dt('Site_dir %site_dir already exists and could not rename it, exiting.', array('%site_dir' => $site_dir)));
    }
  }

  // Prepare the sitedir, cd to it so that we can unpack the baseline package
  // and start the php webserver.
  drush_mkdir($target_site_path);
  // Register dir/files for deletion when we're done.
  if ($cleanup) {
    // Delete site dir.
    drush_register_file_for_deletion($target_site_path, TRUE);
  }

  // Prepare a sites.php if drupal-multisite-host is set.
  if (!empty($context['external-site-host']) && $context['external-site-host'] != $context['drupal-multisite-host']) {
    if ($context['no-generate-sites-php']) {
      // Entry in sites.php needed by auto-generation is disabled (default).
      // Issue a warning and continue.
      drush_log("external-site-host and drupal-multisite-host differs and automatic sites.php generation is disabled via no-generate-sites-php, please make sure sites.php maps the external hostname.", 'warning');
    }
    else {
      // Generate a new sites.php while preserving any existing.
      if (!drake_ci_behat_generate_sitesphp($site_dir, $context)) {
        return FALSE;
      }
    }
  }

  if (file_exists($context['baseline-package'])) {
    if ($context['baseline-type'] == 'aegir') {
      // Unpack the baseline package into the specified target site-path.
      if (!drake_ci_behat_extract_aegir_baseline($site_dir, $target_site_path, $context)) {
        return FALSE;
      }
    } else {
      // Archive Dump.
      if (!drake_ci_behat_extract_arb_baseline($site_dir, $target_site_path, $context)) {
        return FALSE;
      }
    }
  }

  // Create a database based on the profile-name and port-number.
  if (!($db_spec = drake_ci_behat_setup_db($profile, $port, $context, $cleanup))) {
    return FALSE;
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
      // For now just do the same - but other packages such as archive-dump
      // should be supported here.
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
  $db_url = $db_spec['driver'] . '://' . $db_spec['username'] . ':' . $db_spec['password'] . '@' . $db_spec['host'] . '/' . $db_spec['database'];
  $drush_invoke_options['db-url'] = $db_url;

  if (file_exists($sqldump_path)) {
    if (!drake_ci_behat_import_db($sqldump_path, $db_spec, $drush_invoke_options, $target_site_path)) {
      return FALSE;
    }
  }
  else {
    if (!drake_ci_behat_site_install($profile, $db_spec, $site_dir, $drush_invoke_options)) {
      return FALSE;
    }
  }

  $log_file = $output_dir . '/behat-webserver-' . $port . '-' . posix_getpid() . '.log';
  if (!drake_ci_behat_start_webserver($context['drupal-multisite-host'], $port, $log_file, $context)) {
    return FALSE;
  }


  if (!$context['no-behat']) {
    return drake_ci_behat_run_behat($context, $behat_dir, $port, $output_dir, $behat_profiles, $stdout_logfile, $errout_logfile);
  }
  else {
    // Crude hack as we are executing in the background and thus cannot prompt
    // for keyboard input.
    // TODO: This is not a viable solution as the shutdown functions won't get
    // called and all the temp. stuff from the test keeps lying around.
    drush_log(dt('Press CTRL+c to exit', array(), 'OK'));
    while (TRUE) {
      sleep(1000);
    }
    // For codestyle - we'll never get here.
    return TRUE;
  }
}

/**
 * @param $context
 * @param $behat_dir
 * @param $port
 * @param $output_dir
 * @param $behat_profiles
 * @param $descriptorspec
 * @param $stdout_logfile
 * @param $errout_logfile
 * @return bool
 */
function drake_ci_behat_run_behat($context, $behat_dir, $port, $output_dir, $behat_profiles) {
  $behat_config = $context['behat-config'];

  // Create runtime behat-configuration with the video-url logger extension
  // injected.
  $behat_config_full = $behat_dir . '/' . $behat_config;
  if (!file_exists($behat_config_full)) {
    return drake_action_error(dt('Could not update find the behat-config %file.', array('%file' => $behat_config_full)));
  }
  $yaml = new Parser();
  try {
    $config_parsed = $yaml->parse(file_get_contents($behat_config_full));
  } catch (ParseException $e) {
    return drake_action_error(
      dt("Unable to parse the behat-configuration at : %file\n Error-message: %error",
        array(
          '%file'  => $behat_config_full,
          '%error' => $e->getMessage(),
        )
      )
    );
  }
  $config_parsed = drake_ci_behat_update_config($config_parsed, $context, $port);

  $behat_features = $context['behat-features'];
  // The HTML and Junit formatter needs a file and dir respectivly.
  $out_html = $output_dir . '/behat-result.html';
  $out_junit = $output_dir;

  // TODO Loop over profiles here.
  // We only support a single profile for now.
  $behat_profile = $behat_profiles[0];
  if (empty($config_parsed[$behat_profile])) {
    return drake_action_error(dt('Unable to find behat-profile "%profile" in behat-configurationfile', array('%profile' => $behat_profile)));
  }
  // The session-name may have been merged with an existing one in the config
  // so pull it out of the parsed and updated config.
  // TODO: Has to be cleaner way to do this.
  $session_name = $config_parsed[$behat_profile]['extensions']['Behat\MinkExtension\Extension']['selenium2']['capabilities']['name'];

  // Prepare for phantomjs by starting up a process if nessecary.
  if (!empty($config_parsed[$behat_profile]['extensions']['Behat\MinkExtension\Extension']['selenium2']['browser'])
    && $config_parsed[$behat_profile]['extensions']['Behat\MinkExtension\Extension']['selenium2']['browser'] == 'phantomjs'
  ) {
    $process_log_file = $output_dir . '/behat-phantomjs-' . $port . '-' . posix_getpid() . '.log';
    $wd_log_file = $output_dir . '/behat-phantomjs-wd-' . $port . '-' . posix_getpid() . '.log';

    $wd_url = drake_ci_behat_start_phantomjs($process_log_file, $wd_log_file, $context['phantomjs-path']);
    if (!$wd_url) {
      return FALSE;
    }
    else {
      $config_parsed[$behat_profile]['extensions']['Behat\MinkExtension\Extension']['selenium2']['wd_host'] = $wd_url;
    }
  }

  // Hack for now - behat has a problem with capabilities with dashes, so to fix
  // this we use an environment-variable to specify them as it circumvent the
  // faulty code.
  // The function rewrites the runtime config and strips out the capabillites
  // for the specified profile.
  list($behat_proc_env, $config_parsed) = drake_ci_behat_dump_config_to_env($behat_profile, $config_parsed);

  // Prepare a string-version for debugging.
  $behat_proc_env_pretty = array();
  foreach ($behat_proc_env as $key => $value) {
    $behat_proc_env_pretty[] = $key . '=' . $value;
  }
  $behat_proc_env_pretty = implode(':', $behat_proc_env_pretty);

  // Write out the configuration to use for this run. Same dir as original.
  $behat_runtime_config = drake_ci_behat_write_config($config_parsed, dirname($behat_config_full));

  $run_strict = $context['behat-strict'] ? '--strict' : '';
  // Execute behat.
  // TODO: Run multiple behat instances - in sequence to begin with - then maybe
  // in parallel.
  // Use a temporary log file, to avoid buffers being filled.
  $stdout_logfile = $output_dir . '/behat-' . $port . '-' . posix_getpid() . '.log';
  $errout_logfile = $output_dir . '/behat-error-' . $port . '-' . posix_getpid() . '.log';
  drush_register_file_for_deletion($log_file);

  if (empty($context['no-behat-log'])) {
    $descriptorspec = array(
      0 => array('file', '/dev/null', 'r'),
      1 => array('file', $stdout_logfile, 'w'),
      2 => array('file', $errout_logfile, 'w'),
    );
  }
  else {
    $descriptorspec = array();
  }

  $cmd = 'behat -v ' . $run_strict . ' -p ' . escapeshellarg($behat_profile) . ' -c ' . escapeshellarg($behat_runtime_config) . ' -f pretty,junit,html --out ,' . escapeshellarg($out_junit) . ',' . escapeshellarg($out_html) . ' ' . escapeshellarg($behat_features);
  drush_log(dt('Running ' . $cmd . ' in behat-dir: %dir with environment %env', array(
    '%dir' => $behat_dir,
    '%env' => $behat_proc_env_pretty,
  )), 'debug');
  drush_log(dt('Starting behat session named %session', array('%session' => $session_name)), 'ok');
  $behat_process = proc_open($cmd, $descriptorspec, $pipes, $behat_dir, $behat_proc_env);
  register_shutdown_function('_drake_ci_kill_process_shutdown', $behat_process);

  $max_executiontime = $context['max-executiontime'];
  $start = time();

  // Wait a sec.
  sleep(1);
  // Then get the process status.
  if ($behat_process) {
    $proc_status = proc_get_status($behat_process);
  }
  $force_exit = FALSE;
  if ($behat_process && $proc_status['running']) {
    // Wait for the process to stop.
    do {
      sleep(1);

      // TODO: Clean up behat process.
      $proc_status = proc_get_status($behat_process);
      // Halt if max execution-time has passed.
      $force_exit = time() > ($start + ($max_executiontime));
    } while ($proc_status['running'] && $proc_status['pid'] && !$force_exit);

    // Scan the output logfile for video-links, also dump to log.
    // TODO: we only do this to get the progress, would be nicer to just allow
    // it to go to stdout directly.
    $stdout_content = file($stdout_logfile);
    $matches = array();
    foreach ($stdout_content as $line) {
      if (preg_match('#saucelabs.com/jobs#', $line)) {
        $matches[] = $line;
      }
      else {
        drush_log(rtrim($line), 'success');
      }
    }

    $matches_markup = '';
    if (!empty($matches)) {
      // Log the video-url via drush, and generate a html-report.
      // TODO: Inject the URL into the behat html-report instead.
      drush_log("Video URL:", 'success');
      $matches = array_unique($matches);
      $matches_markup = '';
      foreach ($matches as $match) {
        drush_log($match, 'success');
        $matches_markup .= "<li><a href=\"$match\">$match</a>\n";
      }
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

  drush_log(dt('Behat execution completed in %sec seconds, output can be found in %outputdir', array(
    '%sec'       => (time() - $start),
    '%outputdir' => $output_dir
  )), 'ok');
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
 * @param $config_parsed
 * @return string
 */
function drake_ci_behat_write_config($config_parsed, $destination_dir = NULL) {
  if (empty($destination_dir)) {
    $destination_dir = sys_get_temp_dir();
  }

  $behat_runtime_config = tempnam($destination_dir, 'behat-');
  $dumper = new Dumper();
  $dumper->setIndentation(2);
  $yaml = $dumper->dump($config_parsed, 5);
  file_put_contents($behat_runtime_config, $yaml);
  drush_log("Dumping updated behat-configuration to  " . $behat_runtime_config, 'debug');

  // Make sure we delete the temp-file after execution.
  register_shutdown_function(function () use ($behat_runtime_config) {
    echo("Unlinking " . $behat_runtime_config . "\n");
    unlink($behat_runtime_config);
  });
  return $behat_runtime_config;
}

/**
 * Remove the specified profile from the behat-configuration and return an
 * environment-variable that should be used instead.
 * @param $context
 * @param $mink_extension_params
 * @return mixed
 */
function drake_ci_behat_dump_config_to_env($profile, $config_parsed) {
  if (empty($config_parsed[$profile])) {
    // The profile does not exist in the behat configuration.
    return array(array(), $config_parsed);
  }

  // Get the selenium2 configuration for the profile from the configuration.
  $mink_extension_params['selenium2'] = $config_parsed[$profile]['extensions']['Behat\MinkExtension\Extension']['selenium2'];

  // Normally things in the default profile will be overriden by settings in
  // the specific profile behat is running with. ENV settings on the other hand
  // only acts as a default if nothing else is set, in other words settings in
  // the default profile will override ENV settings.
  // To get around this we remove anything from the default profile that might
  // clash.
  foreach ($mink_extension_params['selenium2'] as $key => $value) {
    if (!empty($config_parsed['default']['extensions']['Behat\MinkExtension\Extension']['selenium2'][$key])) {
      unset($config_parsed['default']['extensions']['Behat\MinkExtension\Extension']['selenium2'][$key]);
    }

    // If nothing is left in the default profile under selenium2, remove the key
    // completely.
    if (empty($config_parsed['default']['extensions']['Behat\MinkExtension\Extension']['selenium2'])) {
      unset($config_parsed['default']['extensions']['Behat\MinkExtension\Extension']['selenium2']);
    }
  }

  // The entire selenium2 section has to be blanked out, and replaced with a
  // null capability in order not to clash with some internal mink-config
  // parsing wirdness.
  $config_parsed[$profile]['extensions']['Behat\MinkExtension\Extension']['selenium2'] = array('capabilities' => NULL);

  // Dump the entire configuration into a string for the environment.
  // Base on current environment if any.
  $behat_proc_env = empty($_ENV) ? array() : $_ENV;
  $behat_proc_env['MINK_EXTENSION_PARAMS'] = http_build_query($mink_extension_params);

  return array($behat_proc_env, $config_parsed);
}

/**
 * @param $site_local_uri
 * @param $port
 * @param $context
 * @return bool
 */
function drake_ci_behat_start_webserver($hostname, $port, $log_file, $context) {
  // Get a webserver up and running.
  // Use a temporary log file, to avoid buffers being filled.
  drush_register_file_for_deletion($log_file);
  $descriptorspec = array(
    0 => array('file', '/dev/null', 'r'),
    1 => array('file', '/dev/null', 'w'),
    2 => array('file', $log_file, 'w'),
  );

  // Go via ipv4 - different browers has a tendency to interpet "localhost"
  // differently.
  $listen_addr = $hostname == 'localhost' ? '127.0.0.1' : $hostname;

  $listen_addr .= ':' . $port;
  $cmd = $context['php'] . ' -t ' . $context['root'] . ' -S ' . $listen_addr . ' ' . escapeshellarg(dirname(dirname(__FILE__)) . '/router.php');
  drush_log("Executing command " . $cmd, 'debug');

  // We'd like to use drush runserver instead, but in initial testing runserver
  // would cause core tests to fail on login, while this would not.
  $php_process = proc_open($cmd, $descriptorspec, $pipes, $context['root']);
  register_shutdown_function('_drake_ci_kill_process_shutdown', $php_process);

  // Wait a sec.
  sleep(1);
  // Then check that the server started.
  $proc_status = proc_get_status($php_process);
  if (!$php_process || !$proc_status['running']) {
    return drake_action_error(dt('Could not start internal web server.'));
  }

  drush_log(dt('Webserver running at http://%host:%port',
    array(
      '%host' => $hostname,
      '%port' => $port,
    )
  ), 'ok');
  return TRUE;
}


function drake_ci_behat_start_phantomjs($log_file, $wd_log_file, $executable_path = NULL) {
  if (empty($executable_path)) {
    $executable_path = 'phantomjs';
  }
  $port = mt_rand(40000, 50000);
  // Use a temporary log file, to avoid buffers being filled.
  drush_register_file_for_deletion();
  $descriptorspec = array(
    0 => array('file', '/dev/null', 'r'),
    1 => array('file', '/dev/null', 'w'),
    2 => array('file', $log_file, 'w'),
  );

  // We'd like to use drush runserver instead, but in initial testing runserver
  // would cause core tests to fail on login, while this would not.
  // Phantomjs curiosity, it plain refuses to create a logfile with dashes.
  $cmd = $executable_path . ' --webdriver=' . $port . ' --webdriver-logfile=' . escapeshellarg(strtr($wd_log_file, '-', '_')) . ' --webdriver-loglevel=DEBUG';
  drush_log("Executing command " . $cmd, 'debug');
  $phantom_process = proc_open($cmd, $descriptorspec, $pipes);
  register_shutdown_function('_drake_ci_kill_process_shutdown', $phantom_process);

  // Wait a sec.
  sleep(1);
  // Then check that the server started.
  $proc_status = proc_get_status($phantom_process);
  if (!$phantom_process || !$proc_status['running']) {
    return drake_action_error(dt('Could not start phantomjs.'));
  }

  $wd_url = "http://localhost:$port/wd/hub";

  drush_log(dt('Phantomjs running at WebDriver "%wd_url" %proc',
    array(
      '%wd_url' => $wd_url,
      '%proc' => $phantom_process
    )
  ), 'ok');
  return $wd_url;
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
 * @param $sqldump_path
 * @param $db_spec
 * @param $drush_invoke_options
 * @return bool
 */
function drake_ci_behat_import_db($sqldump_path, $db_spec, $drush_invoke_options, $target_site_path) {
// Database-dump found.
  drush_log(dt('Importing database.sql into the database %dbname', array('%dbname' => $db_spec['database'])), 'ok');
  $success = _drush_sql_query(NULL, $db_spec, $sqldump_path);
  // Import database.
  if (!$success) {
    return drake_action_error(dt('Could not import database-dump from %dump.', array(
      '%dump',
      $sqldump_path
    )));
  }

  // Run updb to bring in any changes since the database where dumped.
  drush_log('Running update-db', 'ok');
  $args = $drush_invoke_options;
  $args[] = "-y";

  $res = drush_invoke_process(NULL, 'updatedb', $args, $args, TRUE);

  if (!$res || $res['error_status'] != 0) {
    return drake_action_error(dt('Error running updatedb.'));
  }
  // Clear cache as the imported database might contains some paths that needs
  // updating.
  drush_log('Flushing site-cache after database-import', 'ok');
  $res = drush_invoke_process(NULL, 'cache-clear', array('all'), $drush_invoke_options, TRUE);

  if (!$res || $res['error_status'] != 0) {
    return drake_action_error(dt('Error clearing cache.'));
  }
  else {
    return TRUE;
  }
}

/**
 * @param $profile
 * @param $port
 * @param $context
 * @param $cleanup
 * @return bool
 */
function drake_ci_behat_setup_db($profile, $port, $context, $cleanup) {
// Create a database for the site.
  $db_spec = array(
    'driver'    => 'mysql',
    'database'  => (strlen($profile) > 8 ? substr($profile, 0, 8) : $profile) . "_" . $port,
    'host'      => 'localhost',
    'username'  => $context['db-su'],
    'password'  => $context['db-su-pw'],
    'port'      => 3306,
    'prefix'    => '',
    'collation' => 'utf8_general_ci',
  );
  // This requires the db-su user to have a CREATE DATABASE grant.
  if (!_drush_sql_create($db_spec)) {
    return drake_action_error(dt('Could not create database %database.', array(
      '%database',
      $db_spec['database']
    )));
  }
  else {
    drush_log(dt('Created temporary database %dbname', array('%dbname' => $db_spec['database'])));
  }

  // Make sure the database gets dropped at exit.
  if ($cleanup) {
    register_shutdown_function('_drake_ci_drop_db_shutdown', $db_spec);
  }
  return $db_spec;
}

/**
 * @param $site_dir
 * @param $target_site_path
 * @param $context
 * @return bool
 */
function drake_ci_behat_extract_aegir_baseline($site_dir, $target_site_path, $context) {
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

/**
 * @param $site_dir
 * @param $target_site_path
 * @param $context
 * @return bool
 */
function drake_ci_behat_extract_arb_baseline($site_dir, $target_site_path, $context) {
//  $oldcwd = getcwd();
//  chdir($target_site_path);

  // Large parts of the following comes from drush_archive_restore. It however
  // does not support extracting a single site into a specific destination so
  // I had to do some rewriting.
  $tmp = drush_tempdir();
  $file = $context['baseline-package'];
  if (!$files = drush_tarball_extract($file, $tmp)) {
    return drush_set_error('DRUSH_ARCHIVE_UNABLE_TO_EXTRACT', dt('Unable to extract site archive tarball to !tmp.', array('!tmp' => $tmp)));
  }

  $manifest = $tmp . '/MANIFEST.ini';
  if (file_exists($manifest)) {
    if (!$ini = parse_ini_file($manifest, TRUE)) {
      return drush_set_error('DRUSH_ARCHIVE_UNABLE_TO_PARSE_MANIFEST', dt('Unable to parse MANIFEST.ini in the archive.'));
    }
  }
  else {
    $ini = drush_archive_guess_manifest($tmp);
  }

  // Global + a single site.
  // TODO, named site.
  if (count($ini) !== 2 && empty($context['archive-site-name'])) {
    return drush_set_error('BASELINE_TO_MANY_SITES', dt('The baseline should only contain a single site!'));
  }

  // First element is global, nip it off.
  $global = array_shift($ini);

  // The rest is a list of sites, only one if archive-site-name i set.
  $site = array_shift($ini);
  $docroot = basename($site['docroot']);


  // We now know where to find the extracted site (under $docroot).
  $site_destination = $target_site_path;

  if (!drush_move_dir("$tmp/$docroot/" . $site['sitedir'], $site_destination, TRUE)) {
    return drush_set_error('DRUSH_ARCHIVE_UNABLE_TO_RESTORE_FILES', dt('Unable to restore site to !dest', array('!dest' => $site_destination)));
  }

  // Restore database.
  $sql_file = $tmp . '/' . $site['database-default-file'];
  // TODO, we should just import it instead of piggybagging on the agir code.
  rename($sql_file, $site_destination . '/database.sql');

    return TRUE;
}

/**
 * Generate a new sites.php while preserving any existing sites.php by moving it
 * out of the way temporarily.
 * @param $context
 * @param $site_dir
 * @return bool
 */
function drake_ci_behat_generate_sitesphp($site_dir, $context) {
  $sites_php = $context['root'] . '/sites/' . 'sites.php';
  if (file_exists($sites_php)) {
    if (!rename($sites_php, $sites_php . "_org")) {
      return drake_action_error(dt('Could not rename %file.', array(
        '%file',
        $sites_php
      )));
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
    return drake_action_error(dt('Could not write to %file, the file needs to be updated as external-site-host differs from drupal-multisite-host.', array(
      '%file',
      $sites_php
    )));
  }
  $buffer = "<?php\n";
  $buffer .= "\$sites['" . $context['external-site-host'] . "'] = '" . $site_dir . "';\n";
  fwrite($fh, $buffer);
  fclose($fh);
  return TRUE;
}

/**
 * Takes in a parsed behat configuration and updates it with overrides from the
 * context.
 *
 * @param $config_parsed
 * @param $context
 * @param $webserver_port
 * @return mixed
 */
function drake_ci_behat_update_config($config_parsed, $context, $webserver_port) {
  $behat_extension_path = dirname(__FILE__) . '/../behat_extensions/log_videourl.php';

  // Prepare the configurations we want to inject.
  // Build up a base_url to use when modifying the configuration.
  $base_url = 'http://';

  if (!empty($context['external-site-host'])) {
    $base_url .= $context['external-site-host'];
  } else {
    $base_url .= $context['drupal-multisite-host'];
  }
  $base_url .= ':';

  if(!empty($context['external-site-port'])) {
    $base_url .= $context['external-site-port'];
  } else {
    $base_url .= $webserver_port;
  }

  if (!empty($context['selenium-wd-host'])) {
    $inject_config['wd_host'] = $context['selenium-wd-host'];
  }

  if (!empty($context['capabilities'])) {
    $inject_config['capabilities'] = $context['capabilities'];
  }
  else {
    // Extract capabillities dynamically.
    // Generate name if not specified.
    if (empty($context['selenium-cap-name'])) {
      $context['selenium-cap-name'] = $context['drupal-multisite-host'] . '-' . $context['profile'] . '-' . date("YmdHis");
    }

    // Strip off the prefix, and we now have the key for the behat-config-file.
    foreach ($context as $key => $value) {
      if (strpos($key, SELENIUM_CAP_PREFIX) === 0 && !empty($value)) {
        $inject_config['capabilities'][substr($key, strlen(SELENIUM_CAP_PREFIX))] = $value;
      }
    }
  }

  // Outer level is the profile.
  foreach ($config_parsed as $profile_name => $profile_configuration) {
    // Make sure this is a potential saucelabs-profile.
    if (!isset($profile_configuration['extensions']['Behat\MinkExtension\Extension']['selenium2'])) {
      continue;
    }

    // Use auto-generated base_url.
    // TODO: If we want to support tests without the internal webserver this
    // might have to be wrapped in a conditional.
    $config_parsed[$profile_name]['extensions']['Behat\MinkExtension\Extension']['base_url'] = $base_url;

    // Add the video-logger extension.
    $config_parsed[$profile_name]['extensions'][$behat_extension_path] = NULL;

    // Add a shortcut to make the following code more readable.
    $selenium_config =& $config_parsed[$profile_name]['extensions']['Behat\MinkExtension\Extension']['selenium2'];

    // Manual merge of two levels of array, the outer is the general selenium
    // config, underneath-that there is a capabilities level.
    // In general the inject values wins, except for the name that gets merged
    // in case of a collision.
    foreach($inject_config as $outer_name => $outer_value) {
      if ($outer_name == 'capabilities') {
        continue;
      }
      $selenium_config[$outer_name] = $outer_value;
    }

    // Merge in the capabilities level.
    if (!empty($inject_config['capabilities'])) {
      foreach($inject_config['capabilities'] as $cap_name => $cap_value) {
        if ($cap_name == 'name' && !empty($selenium_config['capabilities']['name'])) {
          // Prefix with the original value.
          $cap_value = $selenium_config['capabilities']['name'] . ' - ' . $cap_value;
        }
        $selenium_config['capabilities'][$cap_name] = $cap_value;
      }
    }
  }

  return $config_parsed;
}

/**
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
