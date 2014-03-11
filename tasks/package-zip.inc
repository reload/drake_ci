<?php

/**
 * @file
 * Package a build into a timestamped zip-file placed in the root of the site.
 */

/**
 * Task.
 */
$tasks['package-zip'] = array(
  'action' => 'package-zip',
  'files' => fileset('all-no-git'),
  'output-dir' => context_optional('package-output-dir', context('[@self:site:root]')),
  'basename' => context_optional('package-basename', 'package'),
  'prefix' => context_optional('package-prefix', date('Y-m-d-His')),
);

/*
 * Action.
 */
$actions['package-zip'] = array(
  'default_message' => 'Packaging build.',
  'callback' => 'drake_ci_package',
  'parameters' => array(
    'files' => 'Files to package',
    'output-dir' => 'Output directory',
    'basename' => array(
      'description' => 'Destination filename without extension',
      'default' => 'packaged',
    ),
    'prefix' => array(
      'description' => 'Optional prefix to add to the package',
      'default' => '',
    ),
  ),
);

/**
 * Action callback; package a build.
 */
function drake_ci_package($context) {
  if (empty($context['files'])) {
    return drake_action_error(dt('No files specified.'));
  }

  // Prepare the path with trailing slash.
  $output = $context['output-dir'] . (substr($context['output-dir'], -1) == '/' ? '' : '/');
  if (!is_writable($output)) {
    return drake_action_error(dt('Output dir @dir is not writable.', array('@dir' => $output)));
  }
  // Add the prefix, basename and extension.
  $output .= (empty($context['prefix']) ? '' : $context['prefix'] . '-');
  $output .= $context['basename'];
  $output .= '.zip';

  // Prepare the zip-file.
  $zip = new ZipArchive();
  // ZipArchive::CREATE == Create or overwrite.
  $res = $zip->open($output, ZipArchive::CREATE);
  if ($res !== TRUE) {
    return drake_action_error(drake_ci_get_zip_status_string($res));
  }

  // Add files to the archive.
  drush_log(dt('Packaging to @file', array('@file' => $output)), 'status');
  foreach ($context['files'] as $file) {
    $zip->addFile($file->fullPath(), $file->path());
  }
  $zip->close();
  return TRUE;
}

/**
 * Returns a human-readable status.
 *
 * Nabbed from http://www.php.net/manual/en/class.ziparchive.php#108601
 */
function drake_ci_get_zip_status_string($status) {
  switch ((int) $status) {
    case ZipArchive::ER_OK:
      return 'No error';

    case ZipArchive::ER_MULTIDISK:
      return 'Multi-disk zip archives not supported';

    case ZipArchive::ER_RENAME:
      return 'Renaming temporary file failed';

    case ZipArchive::ER_CLOSE:
      return 'Closing zip archive failed';

    case ZipArchive::ER_SEEK:
      return 'Seek error';

    case ZipArchive::ER_READ:
      return 'Read error';

    case ZipArchive::ER_WRITE:
      return 'Write error';

    case ZipArchive::ER_CRC:
      return 'CRC error';

    case ZipArchive::ER_ZIPCLOSED:
      return 'Containing zip archive was closed';

    case ZipArchive::ER_NOENT:
      return 'No such file';

    case ZipArchive::ER_EXISTS:
      return 'File already exists';

    case ZipArchive::ER_OPEN:
      return 'Can\'t open file';

    case ZipArchive::ER_TMPOPEN:
      return 'Failure to create temporary file';

    case ZipArchive::ER_ZLIB:
      return 'Zlib error';

    case ZipArchive::ER_MEMORY:
      return 'Malloc failure';

    case ZipArchive::ER_CHANGED:
      return 'Entry has been changed';

    case ZipArchive::ER_COMPNOTSUPP:
      return 'Compression method not supported';

    case ZipArchive::ER_EOF:
      return 'Premature EOF';

    case ZipArchive::ER_INVAL:
      return 'Invalid argument';

    case ZipArchive::ER_NOZIP:
      return 'Not a zip archive';

    case ZipArchive::ER_INTERNAL:
      return 'Internal error';

    case ZipArchive::ER_INCONS:
      return 'Zip archive inconsistent';

    case ZipArchive::ER_REMOVE:
      return 'Can\'t remove file';

    case ZipArchive::ER_DELETED:
      return 'Entry has been deleted';

    default:
      return dt('Unknown status @status', array('@status' => $status));
  }
}
