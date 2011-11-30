<?php

// $Id: video_upload_widget.inc,v 1.11.4.5 2010/05/21 01:14:54 jhedstrom Exp $

/**
 * @file
 * Video_Upload widget hooks and callbacks.
 *
 * This file is loaded by fileifeld.module and the callbacks it contains are
 * invoked by filefield.module. This happens because video_upload delegates it's
 * CCK hooks and callbacks to filefield in video_upload.module.
 */


/**
 * @defgroup "FileField widget settings callbacks."
 * @{
 *
 * These callbacks are invoked by filefield_widget_settings. Their return
 * values are merged with and will override filefields' widget settings
 * callback's return values.
 *
 * @fixme
 *   allow for more token types other than users
 */
function video_upload_widget_settings_form($widget) {
  $form = module_invoke('filefield', 'widget_settings', 'form', $widget);

  // Allowed file extensions.
  $form['file_extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Allowed upload file extensions.'),
    '#default_value' => is_string($widget['file_extensions']) ? $widget['file_extensions'] : 'mov avi mp4 mpa mpe mpg mpeg qt wmv',
    '#size' => 64,
    '#description' => t('Video filename extenstions allowed by the video provider. Seperate extensions with a space and do not include the leading dot.'),
  );

  $form['use_browser_upload_method'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use the Browser Upload method'),
    '#default_value' => isset($widget['use_browser_upload_method']) ? $widget['use_browser_upload_method'] : FALSE,
    '#description' => t('The <a href="!url">Browser Upload method</a> sends video files directly to YouTube, instead of touching the Drupal server. This saves on storage and bandwidth', array('!url' => url('http://code.google.com/apis/youtube/developers_guide_php.html#BrowserUpload'))),
  );

  
  // Maintenance.
  $form['auto_delete_rejected_videos'] = array(
    '#type' => 'checkbox',
    '#title' => t('Auto-delete rejected videos from account'),
    '#description' => t('When videos are rejected, use cron to automatically remove them from the video provider.'),
    '#default_value' => isset($widget['auto_delete_rejected_videos']) ? $widget['auto_delete_rejected_videos'] : FALSE,
  );
  $form['remove_deleted_videos'] = array(
    '#type' => 'checkbox',
    '#title' => t('Remove videos from provider if node is deleted, or video is changed'),
    '#description' => t("When a node is deleted, or a video is changed/updated the original video will be removed from the provider. If left un-checked, videos will remain on the 3rd-party provider's site."),
    '#default_value' => isset($widget['remove_deleted_videos']) ? $widget['remove_deleted_videos'] : FALSE,
  );

  // Width and height.
  $form['display'] = array(
    '#type' => 'fieldset',
    '#title' => t('Display Options'),
    '#tree' => TRUE,
    '#collapsible' => TRUE,
  );
  $form['display']['default_width'] = array(
    '#type' => 'textfield',
    '#title' => t('Default/Large Width'),
    '#required' => TRUE,
    '#default_value' => $widget['display']['default_width'] ? $widget['display']['default_width'] : '480',
    '#description' => t('The width for the default (large) player'),
  );
  $form['display']['default_height'] = array(
    '#type' => 'textfield',
    '#title' => t('Default/Large Height'),
    '#required' => TRUE,
    '#default_value' => $widget['display']['default_height'] ? $widget['display']['default_height'] : '295',
    '#description' => t('The height for the default (large) player'),
  );
  $form['display']['small_width'] = array(
    '#type' => 'textfield',
    '#title' => t('Small Width'),
    '#required' => TRUE,
    '#default_value' => $widget['display']['small_width'] ? $widget['display']['small_width'] : '240',
    '#description' => t('The width for the small player'),
  );
  $form['display']['small_height'] = array(
    '#type' => 'textfield',
    '#title' => t('Small Height'),
    '#required' => TRUE,
    '#default_value' => $widget['display']['small_height'] ? $widget['display']['small_height'] : '148',
    '#description' => t('The height for the small player'),
  );
  $form['display']['thumb_width'] = array(
    '#type' => 'textfield',
    '#title' => t('Thumbnail Width'),
    '#default_value' => $widget['display']['thumb_width'] ? $widget['display']['thumb_width'] : '128',
  );
  $form['display']['thumb_height'] = array(
    '#type' => 'textfield',
    '#title' => t('Thumbnail Height'),
    '#default_value' => $widget['display']['thumb_height'] ? $widget['display']['thumb_height'] : '72',
  );

  // Default title pattern.
  $form['default_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Default Video Title'),
    '#default_value' => $widget['default_title'],
    '#size' => 64,
    '#maxlength' => 255,
    '#description' => t('Since the videos are initially uploaded prior to node creation, they must contain a default title. This title will only be visible on the provider site, and will be updated once the node is saved, and the video has been processed.'),
  );
  if (module_exists('token')) {
    $form['default_title']['#description'] .= ' '. t('<br />The following tokens can be used in the default title');
    $form['default_title']['#suffix'] = theme('fieldset', array('#children' => theme('token_help', 'user'), '#collapsed' => TRUE, '#collapsible' => TRUE, '#title' => t('Tokens')));
  }

  // Default title synchronization.
  $form['default_title_sync'] = array(
    '#type' => 'radios',
    '#title' => t('Default Video Title Synchronization'),
    '#description' => t('The <em>Video Upload module</em> will attempt to synchronize data with the provider, these options determine the extent of such synchronization'),
    '#options' => array(
      VIDEO_UPLOAD_SYNC => t('Synchronize (overwrite default)'), 
      VIDEO_UPLOAD_SYNC_NONE => t('Don\'t Synchronize (keep default)'),
      VIDEO_UPLOAD_SYNC_APPEND => t('Append node title to default'),
    ),
    '#default_value' => isset($widget['default_title_sync']) ? $widget['default_title_sync'] : VIDEO_UPLOAD_SYNC,
    '#required' => TRUE,
  );

  // Default description.
  $form['default_description'] = array(
    '#type' => 'textarea',
    '#title' => t('Default video description'),
    '#default_value' => $widget['default_description'],
    '#description' => t('Since the videos are initially uploaded prior to node creation, they can contain a default description. This description will only be visible on the provider site, and will be updated with the node teaser once the node is saved and the video has been processed.'),
    '#element_validate' => array('_video_upload_widget_validate_strip_tags'),
  );
  if (module_exists('token')) {
    $form['default_description']['#description'] .= t('<br />The following tokens can be used in the default description');
    $form['default_description']['#suffix'] = theme('fieldset', array('#children' => theme('token_help'), '#collapsed' => TRUE, '#collapsible' => TRUE, '#title' => t('Tokens')));
  }

  // Default description synchronization.
  $form['default_description_sync'] = array(
    '#type' => 'radios',
    '#title' => t('Default Video Description Synchronization'),
    '#description' => t('The <em>Video Upload module</em> will attempt to synchronize data with the provider, these options determine the extent of such synchronization'),
    '#options' => array(
      VIDEO_UPLOAD_SYNC => t('Synchronize (overwrite default)'),
      VIDEO_UPLOAD_SYNC_NONE => t('Don\'t Synchronize (keep default)'),
      VIDEO_UPLOAD_SYNC_APPEND => t('Append Description to default')
    ),
    '#default_value' => isset($widget['default_description_sync']) ? $widget['default_description_sync'] : VIDEO_UPLOAD_SYNC_APPEND,
    '#required' => TRUE,
  );

  // Keywords.
  // @todo - some input validation
  $form['default_keywords'] = array(
    '#type' => 'textfield',
    '#title' => t('Default keywords'),
    '#required' => TRUE,
    '#description' => t('Comma-separated list of keywords (no white space allowed in keywords).<br /><br />Since the videos are initially uploaded prior to node creation, they must contain a default set of keywords. These keywords will only be visible on the provider site, and will be updated with taxonomy terms (if in use on this content type) once the node is saved and the video has been processed.'),
    '#default_value' => $widget['default_keywords'],
    '#element_validate' => array('_video_upload_widget_validate_default_keywords'),
  );
  // default keyword synchronization
  $form['default_keyword_sync'] = array(
    '#type' => 'radios',
    '#title' => t('Default Video Keyword Synchronization'),
    '#description' => t('The <em>Video Upload module</em> will attempt to synchronize data with the provider site. These options determine the extent of such synchronization'),
    '#options' => array(
      VIDEO_UPLOAD_SYNC => t('Synchronize (overwrite default)'),
      VIDEO_UPLOAD_SYNC_NONE => t("Don't Synchronize (keep default)"),
      VIDEO_UPLOAD_SYNC_APPEND => t('Append Taxonomy (if applicable) to default keywords')
    ),
    '#default_value' => isset($widget['default_keyword_sync']) ? $widget['default_keyword_sync'] : VIDEO_UPLOAD_SYNC_APPEND,
    '#required' => TRUE,
  );

  $form['publish_sync'] = array(
    '#type' => 'checkbox',
    '#title' => t('Sync Node Status'),
    '#description' => t('Sync the node published / unpublished state with the video provider.'),
    '#default_value' => isset($widget['publish_sync']) ? $widget['publish_sync'] : FALSE,
  ); 

  // CCK node field for screenshot sync
  if (module_exists('imagefield')) {
    $form['stillframe_field'] = array(
      '#type' => 'textfield',
      '#title' => t('Stillframe Field'),
      '#default_value' => $widget['stillframe_field'],
      '#size' => 64,
      '#maxlength' => 255,
      '#description' => t('The module can optionally download a stillframe and associate it with an imagefield, to become a poster image for the node.'),
    );
  }  
  
  // Merge in the provider settings provided by hook
  $form = array_merge_recursive($form, module_invoke($widget['type'], 'settings', 'form', $widget));

  return $form;
}

/**
 * An array of variables as defined above.
 */
function video_upload_widget_settings_save($widget) {
  $filefield_settings = module_invoke('filefield', 'widget_settings', 'save', $widget);
  $video_upload_settings = array(
    'provider',
    'auto_delete_rejected_videos',
    'remove_deleted_videos',
    'file_extensions',
    'use_browser_upload_method',
    'default_title',
    'default_description',
    'default_keywords',
    'developer_tags',
    'display',
    'default_title_sync',
    'default_description_sync',
    'default_keyword_sync',
    'publish_sync',
    'stillframe_field',
  );
  $video_upload_settings = array_merge($video_upload_settings, module_invoke($widget['widget_type'], 'settings', 'save', $widget));
 
  return array_merge($filefield_settings, $video_upload_settings);
}

/**
 * Validate default keywords setting.
 */
function _video_upload_widget_validate_default_keywords($element, &$form_state) {
  if ($form_state['values']['default_keywords']) {
    // @todo abstract out YouTube. This should simply call a provider callback
    //       instead of enforcing a YouTube-specific keyword format.
    // Keywords can't have spaces in them.
    $check = explode(',', $form_state['values']['default_keywords']);
    foreach ($check as $keyword) {
      $keyword = trim($keyword);
      if (strpos($keyword, ' ') !== FALSE) {
        form_error($element, t("No spaces are allowed in keywords."));
        return;
      }
      $keywords[] = $keyword;
    }
    form_set_value($element, implode(',', $keywords), $form_state);
  }
}

/**
 * Strip all tags out of description fields.
 */
function _video_upload_widget_validate_strip_tags($element, &$form_state) {
  $field = $form_state['values'][$element['#name']];
  form_set_value($element, strip_tags($field), $form_state);
}

/**
 * @} End defgroup "Filefield widget settings callbacks."
 */

/**
 * @defgroup "FileField widget element callbacks.
 * @{
 *
 * The call backs are called by form elements that leverage the
 * filefield_widget_value and filefield_widget_process callbacks.  They will
 * be called after the filefield callbacks and their return values will be
 * merged with the filefield callback's.
 *
 */
function video_upload_widget_value($element, $edit = FALSE) {
  $item = filefield_widget_value($element, $edit);
  if ($edit) {
    $item['video_status'] = isset($edit['video_status']) ? $edit['video_status'] : VIDEO_UPLOAD_STATUS_UPLOAD_PENDING;
    $item['video_status_ts'] = isset($edit['video_status_ts']) ? $edit['video_status_ts'] : $_SERVER['REQUEST_TIME'];
  }
  else {
    $item['video_status'] = VIDEO_UPLOAD_STATUS_UPLOAD_PENDING;
    $item['video_status_ts'] = $_SERVER['REQUEST_TIME'];
  }

  return $item;
}

function video_upload_widget_process($element, $edit, &$form_state, $form) {
  $file = $element['#value'];

  $field = content_fields($element['#field_name'], $element['#type_name']);

  $element['video_status'] = array(
    '#type' => 'hidden',
    '#default_value' => $element['#default_value']['video_status'],
  );
  $element['video_status_ts'] = array(
    '#type' => 'hidden',
    '#default_value' => $element['#default_value']['video_status_ts'],
  );
  $element['video_id'] = array(
    '#type' => 'hidden',
    '#default_value' => $element['#default_value']['video_id'],
  );
  $element['video_provider'] = array(
    '#type' => 'hidden',
    '#default_value' => $element['#default_value']['video_provider'],
  );
  
  if (!empty($field['widget']['stillframe_field'])) {
    $element['stillframe'] = array(
      '#type' => empty($element['#default_value']['video_id']) ? 'hidden' : 'checkbox',
      '#title' => t('Update Stillframe'),
      '#default_value' => FALSE,
      '#attributes' => array('class' => 'video-upload-stillframe'),
    );
  }

  return $element;
}

/**
 * An #element_validate callback for the video_upload_widget field.
 *
 * This is mostly a copy of FileField's widget validate callback.
 */
function video_upload_widget_validate(&$element, &$form_state) {
  // If referencing an existing file, only allow if there are existing
  // references. This prevents unmanaged files (outside of FileField) from
  // being deleted if this node were to be deleted.
  if (!empty($element['fid']['#value'])) {
    $field = content_fields($element['#field_name'], $element['#type_name']);
    if ($file = field_file_load($element['fid']['#value'])) {
      $file = (object) $file;
      if ($file->status == FILE_STATUS_PERMANENT) {
        // TODO: We could use field_file_references() here to reference any file
        // but hook_file_delete() needs to be implemented first.
        $references = module_invoke('video_upload', 'file_references', $file);
        if ($references['video_upload'] == 0) {
          form_error($element, t('Referencing to the file used in the %field field is not allowed.', array('%field' => $element['#title'])));
        }
      }
    }
    else {
      form_error($element, t('The file referenced by the %field field does not exist.', array('%field' => $element['#title'])));
    }
  }
}

/**
 * @} End defgroup "FileField widget element callbacks."
 */

/**
 * @defgroup "CCK/Filefield widget theme functions."
 * @{
 * FormAPI theme function. Theme the output of a video_upload field.
 */
function theme_video_upload_widget($element) {
  return theme('form_element', $element, $element['#children']);
}

function theme_video_upload_widget_preview($item = NULL) {
  $message = theme('video_upload_status_text', $item['video_status']);
  return '<div class="messages">' . $message . '</div>' . theme('video_upload_video', $item['id']);
}

/**
 * @} End defgroup "CCK/Filefield widget theme functions."
 */

