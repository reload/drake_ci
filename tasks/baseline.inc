<?php

/**
 * @file
 * Package a build into a timestamped zip-file placed in the root of the site.
 */

/**
 * Task.
 */
$tasks['create-baseline'] = array(
  'action' => 'create-baseline',
  'output-dir' => context_optional('package-output-dir', context('[@self:site:root]')),
  'basename' => context_optional('package-basename'),
  'site' => context_optional('site', 'default'),
  'prefix' => context_optional('package-prefix'),
);

/*
 * Action.
 */
$actions['create-baseline'] = array(
  'default_message' => 'Create baseline.',
  'callback' => 'drake_baseline_create',
  'parameters' => array(
    'output-dir' => 'Output directory',
    'site' => 'Site to dump',
    'basename' => array(
      'description' => 'Short name that goes into the filename, before the timestamp defaults to a nomalized version of the site-name',
      'default' => NULL,
    ),
    'prefix' => array(
      'description' => 'Optional prefix to add to the package',
      'default' => NULL,
    ),
  ),
);

/**
 * Action callback; package a build.
 */
function drake_baseline_create($context) {
  // Prepare the path with trailing slash.
  $destination = $context['output-dir'] . (substr($context['output-dir'], -1) == '/' ? '' : '/');
  if (!is_writable($destination)) {
    return drake_action_error(dt('Output dir @dir is not writable.', array('@dir' => $destination)));
  }

  // Add the prefix, basename and extension.
  $filename_parts = array();
  if(!empty($context['prefix'])){
    $filename_parts[] = $context['prefix'];
  }

  if ($context['basename'] !== NULL) {
    $basename = $context['basename'];
  } else {
    $drush_invoke_options = array();
    $drush_invoke_options['root'] = $context['root'];
    $drush_invoke_options['exact'] = TRUE;
    $drush_invoke_options['format'] = 'string';

    $site_name = drush_invoke_process(NULL, 'variable-get', array('site_name'), $drush_invoke_options, TRUE);
    if ($site_name !== FALSE) {
      $basename = drake_baseline_normalize($site_name['output']);
    } else {
      $basename = 'unnamed';
    }
    $basename .= '-baseline';
  }

  if(!empty($basename)){
    $filename_parts[] = $basename;
  }

  $filename_parts[] = date('Y-m-d');

  $full_destination = $destination . implode('-', $filename_parts) . '.tar.gz';
  // Handle existing file.
  $counter = 0;
  while (file_exists($full_destination)) {
    $counter++;
    $full_destination = $destination . implode('-', $filename_parts) . "-$counter.tar.gz";
  }

  // Run archive-dump
  drush_log('Running archive-dump', 'ok');
  $drush_invoke_options = array();
  $drush_invoke_options['root'] = $context['root'];
  $drush_invoke_options['destination'] = $full_destination;
  $drush_invoke_options['no-core'] = TRUE;

  $res = drush_invoke_process(NULL, 'archive-dump', array($context['site']), $drush_invoke_options, TRUE);
  if (!$res || $res['error_status'] != 0) {
    return drake_action_error(dt('Error running archive-dump.'));
  }
  return TRUE;
}

function drake_baseline_normalize($name) {
  // Nuke anything non (us) char or numeric.
  return preg_replace('%[^a-zA-Z0-9]%', '', $name);
}
