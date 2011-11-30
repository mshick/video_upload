<?php

// $Id: video_upload.views.inc,v 1.7.2.3.2.2 2010/05/05 00:36:03 jhedstrom Exp $

/**
 * @file video_upload.views.inc
 */

/**
 * Implementation of hook_views_handlers().
 */
function video_upload_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'video_upload') . '/views',
    ),
    'handlers' => array(
      // Filter handlers.
      'views_handler_filter_video_upload_status' => array(
        'parent' => 'views_handler_filter_in_operator',
      ),
    ),
  );
}