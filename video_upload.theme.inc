<?php

// $Id: video_upload.theme.inc,v 1.13.2.8.2.4 2010/05/19 20:22:28 jhedstrom Exp $

/**
 * @file video_upload.theme.inc
 * Theme functions for the Video Upload module.
 */

/**
 * Theme a video
 */
function theme_video_upload_video($video_id, $width = '480', $height = '295', $field = array(), $params = array(), $attributes = array()) {
  if (!$video_id) {
    // Output the video image instead.
    $attributes = array(
      'alt' => t('Video processing'),
      'title' => t('Video processing'),
    );
    return theme('video_upload_missing_image', 'full', $width, $height, $attributes);
  }

  $provider = $field['widget']['provider']['type'];

  return theme('video_upload_provider_' . $provider . '_video', $video_id, $width, $height, $field, $params, $attributes);
}

/**
 * Theme a video link
 */
function theme_video_upload_video_link($item) {
  return theme('video_upload_provider_' . $item['video_provider'] . '_video_link', $item['video_id']);
}

/**
 * Theme a video player
 */
function theme_video_upload_video_player($item) {
  return theme('video_upload_provider_' . $item['video_provider'] . '_player_url', $item['video_id']);
}

/**
 * Theme a video stillframe url
 */
function theme_video_upload_video_stillframe($video_id, $provider) {
  return theme('video_upload_provider_' . $provider . '_video_stillframe', $video_id);
}

/**
 * Theme a thumbnail.
 * @param array $field
 *   CCK field definition
 * @param array $item
 * @param int $nid
 *   Associated node ID
 * @param bool $as_link
 *   Display the thumbnail as link
 * @todo offer random image (YT provides 3)
 */
function theme_video_upload_video_thumb($field = FALSE, $item, $nid = FALSE, $as_link = FALSE) {
  $width = $field['widget']['display']['thumb_width'] ? $field['widget']['display']['thumb_width'] : $field['width'];
  $height = $field['widget']['display']['thumb_height'] ? $field['widget']['display']['thumb_height'] : $field['height'];
  if (video_upload_content_is_empty($item, $field)) {
    return;
  }

  $video_id = $item['video_id'];
  if (!$video_id) {
    // Output the video image instead.
    $attributes = array(
      'alt' => t('Video processing'),
      'title' => t('Video processing'),
    );
    $image = theme('video_upload_missing_image', 'thumbnail', $width, $height, $attributes);

    if ($as_link && $nid) {
      $image = l($image, 'node/' . $nid, array('html' => TRUE));
    }

    return $image;
  }
  else {
    $flag = '';
    if ($item['video_status'] == VIDEO_UPLOAD_STATUS_UNKNOWN) {
      // If the video isn't done processing, throw a flag onto the end of the
      // url so the user's browser doesn't cache the processing thumbnail
      $flag = '?f=' . rand(0, 10);
      $attributes['title'] = t('Video processing');
    }
    $attributes_provider = theme('video_upload_provider_' . $item['video_provider'] . '_video_thumb', $video_id, $flag);
  }

  if ($width) {
    $attributes['width'] = $width;
  }
  if ($height) {
    $attributes['height'] = $height;
  }
  
  $attributes = array_merge($attributes, $attributes_provider);

  $img = '<img ' . drupal_attributes($attributes) . '/>';
  if ($as_link && $nid) {
    $img = l($img, 'node/' . $nid, array('html' => TRUE));
  }
  return $img;
}

/**
 * Theme a specific video upload status message
 * @fixme
 *   This function should be integrated with theme_video_upload_status_text(),
 *   as they do nearly the same thing in slightly different ways
 */
function theme_video_upload_video_status_message($item, $message = NULL) {
  if (!$message) {
    $message = t('Video hasn\'t finished processing yet');
  }
  return '<div class="messages">' . $message . '</div>';
}

/**
 * Theme the video upload admin form into a table/grid
 */
function theme_video_upload_admin_video_form($form) {
  jquery_ui_add('ui.dialog');
  drupal_add_css(libraries_get_path('jquery.ui') ."/themes/base/ui.all.css");
  drupal_add_js(drupal_get_path('module', 'video_upload') . '/video_upload_admin.js');
  
  // add css for this table
  drupal_add_css(drupal_get_path('module', 'video_upload') . '/video_upload_admin.css');


  // Overview table:
  $select_all = $form['videos'] ? theme('table_select_header_cell') : array();
  $header = array($select_all, t('Thumbnail'), t('Title'), t('Id'), t('Node'), t('Field Info'), t('Status'));

  // Options
  $output .= drupal_render($form['options']);

  if (isset($form['id']) && is_array($form['id'])) {
    foreach (element_children($form['id']) as $key) {
      $row = array();
      // checkbox
      $row[] = drupal_render($form['videos'][$key]);
      // thumbnail
      $row[] = drupal_render($form['thumb'][$key]);
      // video title
      $row[] = drupal_render($form['title'][$key]);
      // video id
      $row[] = drupal_render($form['id'][$key]);
      // link to node, if available
      $row[] = $form['node'][$key] ? drupal_render($form['node'][$key]) : t('orphaned');
      // field info, if available
      $row[] = $form['field_name'][$key] ? drupal_render($form['field_name'][$key]) : t('n/a');
      // video status
      $row[] = drupal_render($form['status'][$key]);
      // highlight orphaned videos
      $class =  $form['node'][$key] ? '' : 'video-upload-orphaned';
      $rows[] = array('data' => $row, 'class' => $class);
    }
  }
  else  {
    $rows[] = array(array('data' => t('No videos have been uploaded to this account'), 'colspan' => '6'));
  }

  $output .= theme('table', $header, $rows);
  if ($form['pager']['#value']) {
    $output .= drupal_render($form['pager']);
  }

  $output .= drupal_render($form);
  $output .= '<div id="video-upload-admin-preview"></div>';

  return '<div class="video-upload-admin">' . $output . '</div>';

}

/**
 * Theme the video status text
 * @param mixed $status
 *   A status object or integer
 */
function theme_video_upload_status_text($status) {
  if (is_object($status)) {
    $stat = $status->status;
  }
  else {
    $stat = $status;
  }

  $class = 'video-upload-status';
  switch ($stat) {
    case VIDEO_UPLOAD_STATUS_ORPHANED :
      // @todo: abstract out YouTube
      $status_text = t('Video removed from provider');
      $class .= ' video-upload-status-bad';
      break;
    case VIDEO_UPLOAD_STATUS_BAD :
      $status_text = $status->message ? $status->message : t('Unknown Error');
      $class .= ' video-upload-status-bad';
      break;
    case VIDEO_UPLOAD_STATUS_UPLOAD_PENDING:
      $status_text = t('Processing.');
      $class .= ' video-upload-processing';
      break;
    case VIDEO_UPLOAD_STATUS_STILLFRAME :
    case VIDEO_UPLOAD_STATUS_OK :
      $status_text = t('OK');
      $class .= ' video-upload-status-ok';
      break;
    case VIDEO_UPLOAD_STATUS_OK_SYNCED :
      $status_text = t('Up-to-date');
      $class .= ' video-upload-status-ok-synced';
      break;
    case VIDEO_UPLOAD_STATUS_DELETE :
      $status_text = t('Queued for deletion');
      $class .= ' video-upload-status-delete';
      break;
    case VIDEO_UPLOAD_STATUS_UNKNOWN :
    default :
      $status_text = t('Unknown. This most likely means it has not yet been processed.');
      $class .= ' video-upload-status-unknown';
  }

  $attributes = array(
    'class' => $class,
  );

  return '<span' . drupal_attributes($attributes) . '>' . $status_text . '</span>';
}

/**
 * Theme missing thumbnail image.
 *
 * @param string $style
 *   Thumbnail or full
 * @param integer $width
 * @param integer $height
 * @param array $attributes
 *   html attributes for inside the image tag
 */
function theme_video_upload_missing_image($style = 'full', $width = 425, $height = 350, $attributes = array()) {
  $file = $style == 'full' ? 'video.png' : 'video-thumbnail.png';
  $image = drupal_get_path('module', 'video_upload') . '/images/' . $file;

  $defaults = array(
    'width' => $width,
    'height' => $height,
  );
  $attributes = array_merge($defaults, $attributes);
  $alt = isset($attributes['alt']) ? $attributes['alt'] : t('Video');
  $title = isset($attributes['title']) ? $attributes['title'] : '';
  $output = theme('image', $image, $alt, $title, $attributes, FALSE);  
  return $output;

}

/**
 * @defgroup "CCK Formatter theme functions."
 * @{
 */
function theme_video_upload_formatter_default($element) {
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $display = $field['widget']['display'];
  $item = $element['#item'];
  
  if (video_upload_content_is_empty($item, $field) || $item['video_status'] === VIDEO_UPLOAD_STATUS_DELETE) {
    return;
  }

  if (!$item['video_id']) {
    drupal_set_message(theme('video_upload_status_text', $item['video_status']));
  }

  $output = theme('video_upload_video', $item['video_id'], $display['default_width'], $display['default_height'], $field);
  return $output;

}

function theme_video_upload_formatter_small($element) {
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $display = $field['widget']['display'];
  $item = $element['#item'];
  
  if (video_upload_content_is_empty($item, $field)) {
    return;
  }

  return theme('video_upload_video', $item['video_id'], $display['small_width'], $display['small_height'], $field);
}

function theme_video_upload_formatter_thumb($element) {
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $display = $field['widget']['display'];
  $item = $element['#item'];
  
  if (video_upload_content_is_empty($item, $field)) {
    return;
  }

  return theme('video_upload_video_thumb', $field, $item, $item['nid'], FALSE);
}

function theme_video_upload_formatter_thumb_link($element) {
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $display = $field['widget']['display'];
  $item = $element['#item'];
  
  if (video_upload_content_is_empty($item, $field)) {
    return;
  }

  return theme('video_upload_video_thumb', $field, $item, $item['nid'], TRUE);
}

/**
 * @} End defgroup "CCK Formatter theme functions."
 */
 
 
 /**
 * Preprocess variables for the insert-video-upload.tpl.php file.
 */
function template_preprocess_insert_video_upload(&$vars) {
  $vars['url'] = $vars['item']['filepath'];
  $widget = $vars['widget'];
  $vars['width'] = isset($widget['display']['default_width']) ? $widget['display']['default_width'] : '';
  $vars['height'] = isset($widget['display']['default_height']) ? $widget['display']['default_height'] : '';  
}

