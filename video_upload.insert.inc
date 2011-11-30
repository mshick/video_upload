<?php

/**
 * Implementation of hook_insert_widgets().
 */
function video_upload_insert_widgets() {
  $providers = array_keys(video_upload_widget_info());
  $widgets = array();
  $widgets['video_upload_browser_method_widget'] = array();
  
  foreach ($providers as $provider) {
    $widgets[$provider] = array(
      'wrapper' => '.video-upload-element',
      'fields' => array(
        'description' => 'input[name$="[description]"], textarea[name$="[description]"]',
      )
    );
  }
  
  return $widgets;  
}

/**
 * Implementation of hook_insert_styles().
 */
function video_upload_insert_styles() {
  $insert_styles = array();
  $insert_styles['video_upload'] = array('label' => t('Video player'), 'weight' => -10);
  return $insert_styles;
}

/**
 * Implementation of hook_insert_content().
 */
function video_upload_insert_content($item, $style, $widget) {
  $style_name = $style['name'];
  if ($style_name == 'video_upload') {
    return theme(array('insert_video_upload__' . $widget['provider']['type'], 'insert_video_upload'), $item, $widget, $widget['provider']['type']);  
  }
}
