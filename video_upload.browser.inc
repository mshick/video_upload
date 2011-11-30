<?php
// $Id: video_upload.browser.inc,v 1.1.6.12 2010/06/11 17:04:18 bojanz Exp $

/**
 * @file video_upload.browser.inc
 * Provides functions for the Browser upload method.
 */

/**
 * Implementation of hook_widget().
 *
 * Copied from filefield.module
 */
function _video_upload_browser_method_widget(&$form, &$form_state, $field, $items, $delta = 0) {
  $item = array('list' => $field['list_default'], 'data' => array('description' => ''));
  if (isset($items[$delta])) {
    $item = array_merge($item, $items[$delta]);
  }

  $element = array(
    '#type' => 'video_upload_browser_method_widget',
    '#default_value' => $item,
    '#upload_validators' => array(),
  );
  
  return $element;
}

/**
 * Just like video_upload_widget_value, only without calling filefield
 * (no upload to save).
 */
function video_upload_browser_method_widget_value($element, $edit = false) {
  $field = content_fields($element['#field_name'], $element['#type_name']);  
  // Upload success returns a pseudo-post, grab that here.
  // @todo maybe there's a step I'm missing that turns #post into $edit?
  if (!empty($element['#post']) && !$edit) {
    $delta = $element['#delta'];
    $field_name = $element['#field_name'];  
    $edit = $element['#post'][$field_name][$delta];
    if (!empty($field['list_field'])) {
      $edit['list'] = $field['list_default'];
    }
  }

  if (!$edit) {
    $item = $element['#default_value'];
  }
  else {
    $item = $edit;

    // Checkboxes loose their value when empty.
    // If the list field is present make sure its unchecked value is saved.
    if (!empty($field['list_field']) && empty($edit['list'])) {
      $item['list'] = 0;
    } 
  }

  // If the video_id doesn't exist, don't save anything.
  if (empty($item['video_id'])) {
    $item = array();
    $item['video_id'] = $item['video_status'] = $item['video_status_ts'] = $item['video_provider'] = NULL;
  }
  
  return $item;
}

function video_upload_browser_method_widget_process($element, $edit, &$form_state, $form) {
  drupal_add_js('misc/progress.js');
  $item = $element['#value'];

  $field_name = $element['#field_name'];
  $element['#theme'] = 'video_upload_browser_method_widget_item';

  $field = $form['#field_info'] ? $form['#field_info'][$field_name] : content_fields($element['#field_name'], $element['#type_name']);

  // Title is not necessary for each individual field.
  if ($field['multiple'] > 0) {
    unset($element['#title']);
  }

  $nid = $form['nid']['#value'] ? $form['nid']['#value'] : false;

  // Set up the buttons first since we need to check if they were clicked.
  $element['video_browser_upload_start'] = array(
    '#type' => 'submit',
    '#value' => t('Upload'),
    '#ahah' => array( // with JavaScript
       'path' => 'video-upload/dialog/'.   $element['#id'] .'/'. $element['#field_name'] .'/'. $element['#type_name'] . '/'. (int)$nid . '/' . $element['#delta'],
       'wrapper' => $element['#id'] .'-ahah-wrapper',
       'method' => 'replace',
       'effect' => 'fade',
    ),
    '#field_name' => $element['#field_name'],
    '#delta' => $element['#delta'],
    '#type_name' => $element['#type_name'],
    '#upload_validators' => $element['#upload_validators'],
    '#weight' => 100,
    '#post' => $element['#post'],
  );

  $element['video_browser_remove'] = array(
    // With default CCK edit forms, $element['#parents'] is array($element['#field_name'], $element['#delta']).
    // However, if some module (for example, flexifield) places our widget deeper in the tree, we want to
    // use that information in constructing the button name.
    '#name' => implode('_', $element['#parents']) .'_video_upload_remove',
    '#type' => 'submit',
    '#submit' => array('node_form_submit_build_node'),    
    '#value' => t('Remove'),
    '#ahah' => array( // with JavaScript
      'path' => 'video-upload/dialog/'.   $element['#id'] .'/'. $element['#field_name'] .'/'. $element['#type_name'] . '/'. (int)$nid .'/'. $element['#delta'],
      'wrapper' => $element['#id'] .'-ahah-wrapper',
      'method' => 'replace',
      'effect' => 'fade',
    ),
    '#field_name' => $element['#field_name'],
    '#delta' => $element['#delta'],
    '#weight' => 101,
    '#post' => $element['#post'],
  );  

  // Because the output of this field changes depending on the button clicked,
  // we need to ask FAPI immediately if the remove button was clicked.
  // It's not good that we call this private function, but
  // $form_state['clicked_button'] is only available after this #process
  // callback is finished.
  if (_form_button_was_clicked($element['video_browser_remove'])) {
    // Set status to delete, will be evaluated later for remove_deleted_video setting.
    $item['video_status'] = VIDEO_UPLOAD_STATUS_DELETE;
  }  

  // Set access on the buttons.
  $element['video_browser_upload_start']['#access'] = (empty($item['video_id']) && $item['video_status'] != VIDEO_UPLOAD_STATUS_DELETE);
  $element['video_browser_remove']['#access'] = (!empty($item['video_id']) && $item['video_status'] != VIDEO_UPLOAD_STATUS_DELETE);

  // Set the video id
  $element['video_id'] = array(
    '#type' => 'hidden',
    '#value' => $item['video_id'],
  );

  if (!empty($item['video_id']) && $field) {
    $element['preview'] = array(
      '#type' => 'markup',
      '#value' => theme('video_upload_browser_method_widget_preview', $item, $field),
    );
  }
  else {  
    // No video ID yet, present the upload button.
    $element['video_notification'] = array(
      '#default_value' => '<noscript> <div class="video-upload-browser-noscript">' . t('You need to have JavaScript enabled in order to upload a video.') . '</div> </noscript>'                            
    );
  }
  
  $element['video_status'] = array(
    '#type' => 'hidden',
    '#value' => $item['video_status'],
  );
  $element['video_status_ts'] = array(
    '#type' => 'hidden',
    '#value' => $item['video_status_ts'],
  );
  $element['video_provider'] = array(
    '#type' => 'hidden',
    '#value' => $item['video_provider'],
  );
  
  // placeholder.. will be serialized into the data column. this is a place for widgets
  // to put additional data.
  $element['data'] = array(
    '#tree' => 'true',
    '#access' => !empty($item['video_id']),
  );  

  if (!empty($field['title_field'])) {
    $element['data']['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Title'),
      '#value' => isset($item['data']['title']) ? $item['data']['title'] : '',
      '#type' => variable_get('video_upload_title_type', 'textfield'),
      '#maxlength' => variable_get('video_upload_title_length', 128),
    );
  }

  if (!empty($field['description_field'])) {
    $element['data']['description'] = array(
      '#type' => 'textfield',
      '#title' => t('Description'),
      '#value' => isset($item['data']['description']) ? $item['data']['description'] : '',
      '#type' => variable_get('video_upload_description_type', 'textfield'),
      '#maxlength' => variable_get('video_upload_description_length', 256),
    );
  }

  if (!empty($field['widget']['stillframe_field'])) {
    $element['stillframe'] = array(
      '#type' => empty($item['video_id']) ? 'hidden' : 'checkbox',
      '#title' => t('Update Stillframe'),
      '#default_value' => FALSE,
      '#attributes' => array('class' => 'video-upload-stillframe'),
    );
  }

  if (!empty($field['list_field'])) {
    $element['list'] = array(
      '#type' => empty($item['video_id']) ? 'hidden' : 'checkbox',
      '#title' => t('List'),
      '#value' => isset($item['list']) && !empty($item['video_id']) ? $item['list'] : $field['list_default'],
      '#attributes' => array('class' => 'video-upload-list'),
    );
  } 
  else {
    $element['list'] = array(
      '#type' => 'hidden',
      '#value' => '1',
    );
  }
    
  $element['#attributes']['id'] = $element['#id'] .'-ahah-wrapper';
  $element['#prefix'] = '<div '. drupal_attributes($element['#attributes']) .'>';
  $element['#suffix'] = '</div>'; 
  
  return $element;
}

function theme_video_upload_browser_method_widget_item($element) {
  $output = '';
  $output .= '<div class="video-upload-element clear-block">';

  if (!empty($element['video_id']['#value'])) {
    $output .= '<div class="widget-preview">';
    $output .= drupal_render($element['preview']);
    $output .= '</div>';
  }

  $output .= '<div class="widget-edit video-upload-form">';
  $output .=  drupal_render($element);
  $output .= '</div>';

  $output .= '</div>'; // end video-upload-element

  return $output;
}

/**
 * Here we render a video widget,
 */
function theme_video_upload_browser_method_widget_preview($item, $field) {  
  $status_text = theme('video_upload_status_text', $item['video_status']);
  $status_class = 'video-upload-item-' . $item['video_status'];
  
  $thumb = theme('video_upload_video_thumb', $field, $item, 0, FALSE);
  $link = theme('video_upload_video_link', $item);

  $attributes['class'] = 'video-upload-video-info ' . $status_class;  
  $markup = '<div '. drupal_attributes($attributes) .'>';
  $markup .= '<a href="' . $link . '">' . $thumb . '</a>';
  $markup .= '<div class="video-status">Status: ' . $status_text . '</div>';
  $markup .= '</div>';
  
  return $markup;
}

function theme_video_upload_browser_method_widget($element) {
  drupal_add_js('misc/jquery.form.js');
  drupal_add_js(drupal_get_path('module', 'video_upload') .'/video_upload_browser.js');
  drupal_add_css(drupal_get_path('module', 'video_upload') .'/video_upload_browser.css');

  return theme('video_upload_widget', $element);
}

/**
 * Theme an upload notice
 */
function theme_video_upload_browser_method_widget_notice($provider) {
  return theme('video_upload_provider_' . $provider . '_widget_notice');
}

/**
 * Callback. This is the page that the provider redirects to.
 * The reponse goes to an iframe and gets parsed.
 */
function video_upload_browser_method_success($provider) {
  $status = module_invoke("video_upload_provider_" . $provider, 'success');

  if (isset($_COOKIE['video_upload'])) {
    $cookie = unserialize($_COOKIE['video_upload']);
    foreach ($cookie as $field => $values) {
      $field_name = $field;
      $type_name = $values['type_name'];
      $element_id = $values['element_id'];
      $form_build_id = $values['form_build_id'];
      $delta = $values['delta'];      
    }
  }

  $form_element = _video_upload_browser_method_form_process($type_name, $field_name, $form_build_id, $status, $delta);

  if (!$form_element) {
    print drupal_to_js(array('data' => theme('status_messages')));
    exit;
  }

  $output['field'] = drupal_render($form_element);
  $output['element_id'] = $element_id;

  $GLOBALS['devel_shutdown'] = FALSE;
  print drupal_json(array('status' => TRUE, 'data' => $output));
  exit;  
}


function video_upload_browser_method_dialog($element_id, $field_name, $node_type, $nid, $delta) {
  if ($_POST['op'] === 'Upload') {	 	
    $output = drupal_get_form('video_upload_browser_method_upload_form', $element_id, $field_name, $node_type, $_POST['form_build_id'], $nid, $delta);  
  
    $javascript = drupal_add_js(NULL, NULL);
    $video_browser_ahah_settings = array();
    if (isset($javascript['setting'])) {
      foreach ($javascript['setting'] as $settings) {
        if (isset($settings['ahah'])) {
          foreach ($settings['ahah'] as $id => $ahah_settings) {
            if (strpos($id, 'video-upload-browser-form') === 0) {
              $video_browser_ahah_settings[$id] = $ahah_settings;
            }
          }
        }
      }
    }    

    // Add the AHAH settings needed for our new buttons.
    if (!empty($video_browser_ahah_settings)) {
      $output .= '<script type="text/javascript">jQuery.extend(Drupal.settings.video_upload.ahah, '. drupal_to_js($video_browser_ahah_settings) .');</script>';
    }    
  }
  else {
    $form_element = _video_upload_browser_method_form_process($node_type, $field_name, $_POST['form_build_id'], NULL, $delta);
    $output = drupal_render($form_element);  
  }

  $GLOBALS['devel_shutdown'] = FALSE;
  print drupal_to_js(array('status' => TRUE, 'data' => $output));
  exit;
}

/**
 * Generate the AHAH dialog for upload
 *
 * @param $element_id
 * @param $field
 *   CCK field definition array.
 * @param $form_build_id
 *   Need to pass for post-success function
 * @param $delta
 */
function video_upload_browser_method_upload_form($form_state, $element_id, $field_name, $node_type, $form_build_id, $nid, $delta) {
  $field = content_fields($field_name, $node_type);
  $provider = $field['widget']['provider']['type'];

  // Authenticate to provider.
  $http_client = _video_upload_authenticate(NULL, $provider);

  if (!$http_client) {
    // Unable to authenticate.
    return array();
  }
  
  $connect = _video_upload_connect($http_client, $provider);
  $video = _video_upload_video_entry($provider);

  $node = $nid ? node_load($nid) : false;
   
  $video = _video_upload_construct_media_group($connect, $video, $node, $field, $provider);

  $token_array = _video_upload_get_token_array($connect, $video, $provider);

  $form = module_invoke("video_upload_provider_" . $provider, 'browser_method_upload_form', $token_array);

  $form['#theme'] = 'video_upload_browser_method_widget_item';

  $form['#attributes']['enctype'] = 'multipart/form-data';
  
  $form['upload'] = array(
    '#title' => 'Video Upload',
    '#type' => 'file',
    '#description' => filefield_validate_extensions_help($field['widget']['file_extensions']),
    '#size' => 35,
    '#attributes' => array(
      'accept' => implode(',', array_filter(explode(' ', $field['widget']['file_extensions']))),
    ),
  );

  $form['video_browser_upload_send'] = array(
    '#id' => 'video-upload-browser-form',
    '#attributes' => array('class' => 'video-upload-browser-send'),
    '#type' => 'submit',
    '#value' => t('Send Video File'),
    '#ahah' => array( // with JavaScript
       'selector' => '#video-upload-browser-form',
       'path' => 'video-upload/null',
       'wrapper' => 'edit-upload-wrapper',
       'method' => 'replace',
       'effect' => 'fade',
       'progress' => array(
            'type' => 'bar', 
            'url' => '/video-upload/progress/twistage/' . $token_array['token'],
            'interval' => 5000,
        ),
    ),   
  );

  // Put the upload button directly after the upload field.
  $form['upload']['#field_suffix'] = drupal_render($form['video_browser_upload_send']);
  $form['upload']['#field_suffix'] .=  theme('video_upload_browser_method_widget_notice', $provider);
  $form['upload']['#theme'] = 'video_upload_browser_method_widget_file';
  
  $form['#attributes']['id'] = $element_id .'-video-browser-upload';
  $form['#attributes']['class'] = 'video-upload-new-upload';
  $form['#prefix'] = '<div '. drupal_attributes($form['#attributes']) .'>';
  $form['#suffix'] = '</div>'; 

  // Set a cookie with data necessary to recreate the new form via FAPI
  $cookie[$field_name] = array();
  $cookie[$field_name]['type_name'] = $node_type;
  $cookie[$field_name]['element_id'] = $element_id;
  $cookie[$field_name]['form_build_id'] = $form_build_id;
  $cookie[$field_name]['delta'] = $delta;  
  $cookie = serialize($cookie);
  setcookie('video_upload', $cookie, time()+3600*24, '/');

  return $form;
}

function theme_video_upload_browser_method_widget_file($element) {
  $output = '';

  $output .= '<div id="video-upload-browser-form" class="video-upload-browser-upload clear-block">';

  if (isset($element['#field_prefix'])) {
    $output .= $element['#field_prefix'];
  }

  _form_set_class($element, array('form-file'));
  $output .= '<input type="file" name="'. $element['#name'] .'"'. ($element['#attributes'] ? ' '. drupal_attributes($element['#attributes']) : '') .' id="'. $element['#id'] .'" size="'. $element['#size'] ."\" />\n";

  if (isset($element['#field_suffix'])) {
    $output .= $element['#field_suffix'];
  }

  $output .= '</div>';

  return theme('form_element', $element, $output);
}

function _video_upload_browser_method_form_process($type_name, $field_name, $form_build_id, $status = false, $delta) {
  // Build the new form.
  $form_state = array('submitted' => FALSE);
  $form = form_get_cache($form_build_id, $form_state);

  if (!$form) {
    // Invalid form_build_id.
    drupal_set_message(t('An unrecoverable error occurred. This form was missing from the server cache. Try reloading the page and submitting again.'), 'error');
    return false;
  }

  // Build the form. This calls the file field's #value_callback function and
  // saves the uploaded file. Since this form is already marked as cached
  // (the #cache property is TRUE), the cache is updated automatically and we
  // don't need to call form_set_cache().
  $args = $form['#parameters'];
  $form_id = array_shift($args);
  
  if ($_POST) {
    // If there was a real POST use it, we're generating the upload form
    $post = $_POST;
  }
  elseif ($status) {
    // If no POST this is our success callback and we create a pseudo-POST
    $post = array( $field_name => array( 
                      $delta => array( 
                        'video_id' => $status['video_id'],
                        'video_status' => $status['video_status'],
                        'video_status_ts' => time(),
                        'video_provider' => $status['video_provider'],   
                        '_weight' => $delta,
                      ), 
                    ), 
                  );
  }
  else {
    // Nothing to post
    drupal_set_message(t('An unrecoverable error occurred. This form did not post properly.'), 'error');  
    return false;
  }
  
  $form['#post'] = $post;
  
  $form = form_builder($form_id, $form, $form_state);

  // Update the cached form with the new element at the right place in the form.
  if (module_exists('fieldgroup') && ($group_name = _fieldgroup_field_get_group($type_name, $field_name))) {
    if (isset($form['#multigroups']) && isset($form['#multigroups'][$group_name][$field_name])) {
      $form_element = $form[$group_name][$delta][$field_name];
    }
    else {
      $form_element = $form[$group_name][$field_name][$delta];
    }
  }
  else {
    $form_element = $form[$field_name][$delta];
  }

  if (isset($form_element['_weight'])) {
    unset($form_element['_weight']);
  }

  return $form_element;
}

/**
 * Menu callback for upload progress, defines hook_progress
 */
function video_upload_browser_method_progress($provider, $key) {
  $progress = module_invoke("video_upload_provider_" . $provider, 'progress', $key);
  drupal_json($progress);
}