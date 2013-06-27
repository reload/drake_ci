<?php

/**
 * @file
 * Generic code checking/testing and analysis tasks.
 */

$api = 1;

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
    '**/*.field_group.inc',
    '**/*.layouts.inc',
    '**/*.pages_default.inc',
    '**/*.panels_default.inc',
    '**/*.strongarm.inc',
    '**/*.views_default.inc',
  ),
);

$filesets['contrib'] = array(
  'include' => array(
    '**/contrib/**',
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
 */
$filesets['php-custom'] = array(
  'dir' => context('root'),
  'include' => array(
    'php',
    'no-php-generated',
    'no-core',
    'no-contrib',
  ),
);

$filesets['js-custom'] = array(
  'dir' => context('root'),
  'include' => array(
    'js',
    'no-core',
    'no-contrib',
  ),
);

$filesets['css-custom'] = array(
  'dir' => context('root'),
  'include' => array(
    'css',
    'no-core',
    'no-contrib',
  ),
);

/**
 * It's here things get interesting, here the actual testing actions are
 * defined.
 */

/**
 * PHP lint action. Runs the files through PHP to check for syntax errors.
 */
$actions['php-lint'] = array(
  'default_message' => 'PHP linting files',
  'callback' => 'drake_php_lint',
  'parameters' => array(
    'files' => 'Files to lint.',
    'verbose' => array(
      'description' => 'Print all files processed.',
      'default' => FALSE,
    ),
  ),
);

/**
 * Action callback; lint PHP files.
 */
function drake_php_lint($context) {
  foreach ($context['files'] as $file) {
    if ($context['verbose']) {
      drush_log(dt('Linting  @file', array('@file' => $file->path())), 'status');
    }
    // @todo the following makes PHP report everything, including deprecated
    // code. Add as an option.
    // $command .= '-d error_reporting=32767 ';
    drush_shell_exec('php 2>&1 -n -l "%s"', $file);
    $messages = drush_shell_exec_output();

    $bad_files = array();
    foreach ($messages as $message) {
      if (trim($message) == '') {
        continue;
      }
      // @todo: if checking for deprecated code:
      //   preg_match('/^(.*)Deprecated:/', $message)
      if (!preg_match('/^No syntax errors detected/', $message)) {
        if (!isset($bad_files[(string) $file])) {
          $bad_files[(string) $file] = array();
        }

        array_push($bad_files, $message);
        drush_log($message, 'error');
      }
    }
    if (count($bad_files)) {
      drake_action_error('Syntax error in files.');
      return;
    }
  }
}


/**
 * PHP debug statement check action. Greps files for common debug statements.
 */
$actions['php-debug'] = array(
  'default_message' => 'Checking PHP files for debug statements',
  'callback' => 'drake_php_debug',
  'parameters' => array(
    'files' => 'Files to check for debug statements.',
    'verbose' => array(
      'description' => 'Print all files processed.',
      'default' => FALSE,
    ),
  ),
);

/**
 * Action callback; check files for debugging statements.
 */
function drake_php_debug($context) {
  // @todo Make this configurable through the action.
  $debug = array(
    ' dsm\(',
    ' dpm\(',
    ' dpr\(',
    ' dprint_r\(',
    ' db_queryd\(',
    ' krumo',
    ' kpr\(',
    ' kprint_r\(',
    ' var_dump\(',
    ' dd\(',
    ' drupal_debug\(',
    ' dpq\(',

  );
  $overall_status = 'ok';

  foreach ($context['files'] as $file) {
    if ($context['verbose']) {
      drush_log(dt('Checking @file', array('@file' => $file->path())), 'status');
    }
    drush_shell_exec('grep -nHE "(%s)" "%s"', implode('|', $debug), $file);
    $messages = drush_shell_exec_output();

    $bad_files = array();
    foreach ($messages as $message) {
      if (trim($message) == '') {
        continue;
      }
      array_push($bad_files, $message);
      drush_log($message, 'error');
    }
    if (count($bad_files)) {
      drake_action_error('Debug statements found in files.');
      return;
    }
  }
}

/**
 * JS lint action. Runs the files through JavaScriptLint to check for syntax
 * errors.
 */
$actions['js-lint'] = array(
  'default_message' => 'PHP linting files',
  'callback' => 'drake_js_lint',
  'parameters' => array(
    'files' => 'Files to lint.',
    'verbose' => array(
      'description' => 'Print all files processed.',
      'default' => FALSE,
    ),
  ),
);

/**
 * Action callback; Check JS files for syntax errors.
 */
function drake_js_lint($context) {
  $overall_status = 'ok';

  foreach ($context['files'] as $file) {
    if ($context['verbose']) {
      drush_log(dt('Linting  @file', array('@file' => $file->path())), 'status');
    }
    drush_shell_exec('jsl 2>&1 -nologo -nofilelisting -process "%s"', $file);
    $messages = drush_shell_exec_output();
    if (!preg_match('/^(\d+) error(.*?), (\d+) warning/', end($messages), $matches)) {
      drush_log(dt('Unexpected response from jsl: @cmd - @result',
          array(
            '@cmd' => sprintf('jsl 2>&1 -nologo -nofilelisting -process "%s"', $file),
            '@result' => implode("\n", $messages),
          )), 'error');
    }

    array_pop($messages);
    $messages = array_filter($messages);

    $status = $matches[1] > 0 ? 'error' : ($matches[3] > 0 ? 'warning' : 'ok');
    switch ($status) {
      case 'error':
        $overall_status = $status;
        drush_log(implode("\n", $messages), $status);
        break 2;

      case 'warning':
        $overall_status = $overall_status === 'error' ? $overall_status : $status;
        drush_log(implode("\n", $messages), $status);
        break;

      case 'ok':
      default:
        break;
    }
  }
  if ($overall_status === 'error') {
    drake_action_error('Syntax error in files.');
    return;
  }
}


/**
 * JS debug statement check action. Greps files for common debug statements.
 */
$actions['js-debug'] = array(
  'default_message' => 'Checking JS files for debug statements',
  'callback' => 'drake_js_debug',
  'parameters' => array(
    'files' => 'Files to check for debug statements.',
    'verbose' => array(
      'description' => 'Print all files processed.',
      'default' => FALSE,
    ),
  ),
);

/**
 * Action callback; check JS files for common debug statements.
 */
function drake_js_debug($context) {
  // @todo Make this configurable through the action.
  $debug = array(
    ' console.log\(',
  );
  $overall_status = 'ok';

  foreach ($context['files'] as $file) {
    if ($context['verbose']) {
      drush_log(dt('Checking @file', array('@file' => $file->path())), 'status');
    }
    drush_shell_exec('grep -nHE "(%s)" "%s"', implode('|', $debug), $file);
    $messages = drush_shell_exec_output();

    $bad_files = array();
    foreach ($messages as $message) {
      if (trim($message) == '') {
        continue;
      }
      array_push($bad_files, $message);
      drush_log($message, 'error');
    }
    if (count($bad_files)) {
      drake_action_error('Debug statements found in files.');
      return;
    }
  }
}

/**
 * PHPMD action. Runs the files through PHPMD to check for potential problems.
 */
$actions['php-md'] = array(
  'default_message' => 'PHP mess detection',
  'callback' => 'drake_php_md',
  'parameters' => array(
    'files' => 'Files to check.',
    'verbose' => array(
      'description' => 'Print all files processed.',
      'default' => FALSE,
    ),
  ),
);

/**
 * Action callback; check PHP files for protential problems.
 */
function drake_php_md($context) {
  foreach ($context['files'] as $file) {
    if ($context['verbose']) {
      drush_log(dt('Mess detecting @file', array('@file' => $file->path())), 'status');
    }
    drush_shell_exec('phpmd 2>&1 "%s" text codesize,controversial,design,naming,unusedcode', $file);
    $messages = drush_shell_exec_output();

    // Remove empty lines.
    $messages = array_filter($messages);

    if (!empty($messages)) {
      foreach ($messages as $message) {
        drush_log($message, 'warning');
      }
    }
  }
}

/**
 * PHPCPD action. Runs the files through PHPCPD to check for duplicate code.
 */
$actions['php-cpd'] = array(
  'default_message' => 'PHP copy/paste detection',
  'callback' => 'drake_php_cpd',
  'parameters' => array(
    'files' => 'Files to check.',
    'verbose' => array(
      'description' => 'Print all files processed.',
      'default' => FALSE,
    ),
  ),
);


/**
 * Action callback; check PHP files for duplicate code.
 */
function drake_php_cpd($context) {
  foreach ($context['files'] as $file) {
    if ($context['verbose']) {
      drush_log(dt('Copy/paste detecting @file', array('@file' => $file->path())), 'status');
    }
    drush_shell_exec('phpcpd 2>&1 "%s"', $file);
    $messages = drush_shell_exec_output();

    // Get status from the 3rd last line of message
    // @fixme Too flaky assuming 3rd last line is duplication status?
    if (count($messages) < 5 || !preg_match('/^(\d+\.\d+)\% duplicated/', $messages[count($messages) - 3], $matches)) {
      drush_log(dt('Unexpected response from phpcpd: @cmd - @result',
          array(
            '@cmd' => sprintf('phpcpd 2>&1 "%s"', $file),
            '@result' => implode("\n", $messages),
          )), 'error');
    }

    // The first and last two lines are irrelevant.
    $messages = array_slice($messages, 2, -2);

    // Higher than 0% duplication?
    if ($matches[1] > 0) {
      foreach ($messages as $message) {
        drush_log($message, 'warning');
      }
    }
  }
}

/**
 * PHPCS action. Runs the files through PHPCS to check coding style.
 */
$actions['php-cs'] = array(
  'default_message' => 'PHP code sniffer',
  'callback' => 'drake_php_cs',
  'parameters' => array(
    'files' => 'Files to check.',
    'verbose' => array(
      'description' => 'Print all files processed.',
      'default' => FALSE,
    ),
    'standard' => array(
      'description' => 'The coding standard files must conform to.',
      'default' => 'Drupal',
    ),
    'encoding' => array(
      'description' => 'The encoding of the files to check.',
      'default' => 'UTF8',
    ),
  ),
);

/**
 * Action callback; check PHP files for coding style.
 */
function drake_php_cs($context) {
  foreach ($context['files'] as $file) {
    if ($context['verbose']) {
      drush_log(dt('Code sniffing @file', array('@file' => $file->path())), 'status');
    }
    drush_shell_exec('phpcs --standard=%s --encoding=%s 2>&1 "%s"', $context['standard'], $context['encoding'], $file);
    $messages = drush_shell_exec_output();

    // Get status from the 3rd last line of message
    // @fixme Too flaky assuming 3rd last line is duplication status?
    if (count($messages) < 2 || !preg_match('/^Time: (.*?) seconds, Memory: (.*?)/', $messages[count($messages) - 2])) {
      drush_log(dt('Unexpected response from phpcs: @cmd - @result',
          array(
            '@cmd' => sprintf('phpcs --standard=%s --encoding=%s 2>&1 "%s"', $context['standard'], $context['encoding'], $file),
            '@result' => implode("\n", $messages),
          )), 'error');
    }

    // The last two lines are irrelevant.
    $messages = array_slice($messages, 0, -2);

    // Any messages left?
    if (!empty($messages)) {
      drush_log(implode("\n", $messages), 'warning');
    }
  }
}