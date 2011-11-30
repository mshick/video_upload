<?php

// $Id: video_upload.admin.inc,v 1.13.2.8.2.6 2010/05/15 17:45:13 bojanz Exp $

/**
 * @file video_upload.admin.inc
 */

/**
 * Obtain a list of all fields using the video upload module.
 */
function _video_upload_relevant_fields() {
  $fields = content_fields();
  $video_upload_fields = array();
  foreach ($fields as $field => $config) {
    if (($config['type'] == 'video_upload') && strpos($config['widget']['type'], 'video_upload') === 0) {
      $video_upload_fields[$field] = $config;
      $video_upload_fields[$field]['database_information'] = content_database_info($config);
      $video_upload_fields[$field]['provider'] = $config['widget']['provider']['type'];
    }
  }
  return $video_upload_fields;
}

/**
 * Callback function for admin/settings/video-upload
 * @return
 *   Returns an array defining the admin settings form
 */
function _video_upload_admin_settings_form() {
  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General Settings'),
  );
  $form['general']['video_upload_remove_uploaded_files'] = array(
    '#type' => 'checkbox',
    '#title' => t('Remove uploaded files'),
    '#description' => t('After the uploaded files have been transfered to the provider (YouTube, for example), delete them from local storage.'),
    '#default_value' => variable_get('video_upload_remove_uploaded_files', FALSE),
  );
  
  return system_settings_form($form);
}


/**
 * Submit handler for settings forms
 */
function _video_upload_admin_settings_form_submit($form, $form_state) {
  if ($form_state['values']['video_upload_resync']) {
    _video_upload_resync_videos($form_state['values']['video_upload_provider']);
  }

}


/**
 * Menu callback: administer video
 */
function video_upload_admin_video($provider = false, $options = NULL) {
  if ($_POST['operation'] == 'delete' && $_POST['videos']) {
    return drupal_get_form('video_upload_admin_video_delete_confirm');
  }
  else if ($_POST['operation'] == 'attach' && $_POST['videos']) {
    return drupal_get_form('video_upload_admin_video_attach_confirm');
  }
  else if ($_POST['operation'] == 'create' && $_POST['videos']) {
    return drupal_get_form('video_upload_admin_video_create_confirm');
  }  
  $output = drupal_get_form('video_upload_admin_video_form', $provider, $options);
  return $output;
}

/**
 * Video deletion confirmation
 */
function video_upload_admin_video_delete_confirm() {
  $edit = $_POST;

  $form['videos'] = array(
    '#prefix' => '<ul>', 
    '#suffix' => '</ul>', 
    '#tree' => TRUE
  );

  // array_filter returns only elements with TRUE values
  foreach (array_filter($edit['videos']) as $vid => $value) {
    $form['videos'][$vid] = array(
      '#type' => 'hidden',
      '#default_value' => $edit['provider'],
      '#prefix' => '<li>',
      '#suffix' => check_plain($edit['vtitle-' . $vid]) . '</li>' . "\n",
    );
  }
  $form['operation'] = array('#type' => 'hidden', '#value' => 'delete');

  return confirm_form(
    $form,
    t('Are you sure you want to delete these videos?'),
    'admin/content/video-upload', t('This action cannot be undone.'),
    t('Delete all'), t('Cancel')
  );
}

/**
 * Delete videos after confirmation.
 */
function video_upload_admin_video_delete_confirm_submit($form_id, $form_values) {
  if ($form_values['values']['confirm']) {
    foreach ($form_values['values']['videos'] as $video_id => $provider) {
      $connection = _video_upload_connect(TRUE, $provider);
      $video = _video_upload_get_video_object($connection, $video_id, $provider);
      if ($video) {
        _video_upload_delete_video_remote($connection, $video, $provider);
        drupal_set_message(t('Video %id has been removed from %provider', array('%id' => $video_id, '%provider' => $provider)));
      }
    }
    drupal_set_message(t('The videos have been deleted'));
  }

  return 'admin/content/video-upload';
}

/**
 * AHAH helper to build node select form
 */
function video_upload_node_attach_ahah() {
  $field_name = $_POST['field_name'];
  $form = array();
  $form['nodereference'] = array(
    '#type' => 'textfield',
    '#title' => t('Choose a node to modify'),
    '#autocomplete_path' => 'nodereference/autocomplete/' . $field_name,
    '#default_value' => '',
  ); 
  $output = _video_upload_ahah_render($form, 'nodereference');
  print drupal_to_js(array('data' => $output, 'status' => true));
  exit();
}

/**
 * AHAH helper to build node create form
 */
function video_upload_node_create_ahah() {
  $field_name = $_POST['field_name'];
  
  foreach($_POST as $key => $type) {
    if(strpos($key, '|')) {
      $arr = explode('|', $key);
      $field_name = $arr[1];
      $field_options[$field_name] = $field_name;
      $types[$type] = $field_options;
    }
  }
  
  $form = array();
  foreach($types as $type => $field_options) {
    $form['node_field'] = array(
      '#title' => t('Video Upload field'),
      '#type' => 'select',
      '#options' => array_map('check_plain', $field_options),
      '#title' => t('Choose a field to create a video in.'),
      '#prefix' => '<div class="video-upload field-select '. $type .'">',
      '#suffix' => '</div>',
    );   
  }
  $output = _video_upload_ahah_render($form, 'node_fields');
  print drupal_to_js(array('data' => $output, 'status' => true));
  exit();

}

function _video_upload_ahah_render($fields, $name) {
  $form_state = array('submitted' => FALSE);
  $form_build_id = $_POST['form_build_id'];
  $form = form_get_cache($form_build_id, $form_state);
  $form[$name] = $fields;
  form_set_cache($form_build_id, $form, $form_state);
  $form += array(
    '#post' => $_POST,
    '#programmed' => FALSE,
  );
  // Rebuild the form.
  $form = form_builder($_POST['form_id'], $form, $form_state);

  // Render the new output.
  $new_form = $form[$name];
  return drupal_render($new_form); 
}

/**
 * Video append confirmation
 */
function video_upload_admin_video_attach_confirm() {
  $edit = $_POST;
  $provider = $edit['provider'];
  
  $form['videos'] = array(
    '#prefix' => '<ul>', 
    '#suffix' => '</ul>', 
    '#tree' => TRUE
  );

  // array_filter returns only elements with TRUE values
  foreach (array_filter($edit['videos']) as $vid => $value) {
    $form['videos'][$vid] = array(
      '#type' => 'hidden',
      '#default_value' => $provider,
      '#prefix' => '<li>',
      '#suffix' => check_plain($edit['vtitle-' . $vid]) . '</li>' . "\n",
    );
  }

  //get a list of all video upload fields with this provider specified
  $relevant_fields = array();

  $fields = _video_upload_relevant_fields();
  
  foreach ($fields as $field_name => $config) {
    if ($config['provider'] === $provider) {
      $relevant_fields[$field_name] = $field_name;
    }
  }

  if(count($relevant_fields) > 1) {
    // if we have multiple video upload fields, give select list
    $form['field_name'] = array(
      '#type' => 'select',
      '#title' => t('Select a video_upload field'),
      '#options' => array_map('check_plain', $relevant_fields),
      '#suffix' => '<div id="nodereference-ahah-wrapper"></div>',
      '#ahah' => array( // with JavaScript
         'path' => 'video-upload/attach',
         'wrapper' => 'nodereference-ahah-wrapper',
         'method' => 'replace',
         'effect' => 'fade',
      ),      
    );   
  }
  else {
    // if we just have one, it's this easy
    $relevant_field_name = array_pop($relevant_fields);
    $form['field_name'] = array(
      '#type' => 'hidden',
      '#default_value' => $relevant_field_name,
    );
    
    $form['nodereference'] = array(
      '#type' => 'textfield',
      '#title' => t('Choose a node to modify'),
      '#autocomplete_path' => 'nodereference/autocomplete/' . $relevant_field_name,
      '#default_value' => '',
    ); 
  }

  $form['operation'] = array('#type' => 'hidden', '#value' => 'attach');
 
  return confirm_form(
    $form,
    t('Are you sure you want to attach these videos?'),
    'admin/content/video-upload', t('This can be modified later.'),
    t('Attach all'), t('Cancel')
  );
}

/**
 * Go to node edit screen
 */
function video_upload_admin_video_attach_confirm_submit($form_id, $form_values) {
  $values = $form_values['values'];
  $value = $values['nodereference'];
  $matches = array();
  preg_match('/^(?:\s*|(.*) )?\[\s*nid\s*:\s*(\d+)\s*\]$/', $value, $matches);

  if (!empty($matches)) {
    // Explicit [nid:n].
    list(, $title, $nid) = $matches;
    if (!empty($title) && ($n = node_load($nid)) && trim($title) != trim($n->title)) {
      form_error($element[$field_key], t('%name: title mismatch. Please check your selection.', array('%name' => t($field['widget']['label']))));
    }
  }

  $field_name = $values['field_name'];

  if ($values['confirm']) {
    foreach ($values['videos'] as $video_id => $provider) {
      $query[$field_name][$video_id] = VIDEO_UPLOAD_STATUS_OK;
    }
    drupal_set_message(t('Attaching selected videos to this node'));
  }
  $attach = http_build_query($query, NULL, '&');
  drupal_goto('node/' . $nid . '/edit', $attach);
}

/**
 * Video create confirmation
 */
function video_upload_admin_video_create_confirm() {
  $edit = $_POST;

  $form['videos'] = array(
    '#prefix' => '<ul>', 
    '#suffix' => '</ul>', 
    '#tree' => TRUE
  );
  // array_filter returns only elements with TRUE values
  foreach (array_filter($edit['videos']) as $vid => $value) {
    $form['videos'][$vid] = array(
      '#type' => 'hidden',
      '#default_value' => $value,
      '#prefix' => '<li>',
      '#suffix' => check_plain($edit['vtitle-' . $vid]) . '</li>' . "\n",
    );
  }

  //get a list of all video upload fields with this provider specified
  $relevant_fields = array();

  $fields = _video_upload_relevant_fields();
  foreach ($fields as $field_name => $field) {
    if ($config['provider'] === $provider) {
      $type = $field['type_name'];
      $options[$type] = $type;
      $field_options[$field_name] = $field_name;
      $types[$type] = $field_options;
    }
  }

  if(count($types) > 1) {
    $form['node_type'] = array(
      '#type' => 'select',
      '#title' => t('Node type'),
      '#options' => array_map('check_plain', $options),
      '#suffix' => '<div id="node-fields-ahah-wrapper"></div>',
      '#ahah' => array( // with JavaScript
         'path' => 'video-upload/create',
         'wrapper' => 'node-fields-ahah-wrapper',
         'method' => 'after',
         'effect' => 'fade',
      ), 
    );
    foreach($types as $type => $field_options) {
      foreach ($field_options as $field) {
        $form['node_field'][$type . '|' . $field] = array(
          '#type' => 'hidden',
          '#default_value' => $type,
        );
      }
    }
  } else {
    // if we just have one, it's this easy
    $type = key($types);
    $form['node_type'] = array(
      '#type' => 'hidden',
      '#default_value' => $type,
    );
    foreach($types as $type => $field_options) {
      $form['node_field'] = array(
        '#title' => t('Video Upload field'),
        '#type' => 'select',
        '#options' => array_map('check_plain', $field_options),
        '#title' => t('Choose a field to create a video in.'),
        '#prefix' => '<div class="video-upload field-select '. $type .'">',
        '#suffix' => '</div>',
      );   
    }
  }

  $form['operation'] = array('#type' => 'hidden', '#value' => 'create');
  
  return confirm_form(
    $form,
    t('Are you sure you want to create a new node with these videos?'),
    'admin/content/video-upload', t('This action can be undone later.'),
    t('Create node'), t('Cancel')
  );
}

/**
 * Go to node creation screen
 */
function video_upload_admin_video_create_confirm_submit($form_id, $form_values) {
  $values = $form_values['values'];
  $type = str_replace('_', '-', $values['node_type']);
  $field = $values['node_field'];
  
  if ($values['confirm']) {
    foreach ($values['videos'] as $vid => $value) {
      $query[$field][$vid] = VIDEO_UPLOAD_STATUS_OK;
    }
    drupal_set_message(t('Creating a new node with selected videos'));
  }
  
  $create = http_build_query($query, NULL, '&');
  drupal_goto('node/add/' . $type, $create);
}


/**
 * Process video admin form
 */
function video_upload_admin_video_form_submit($form_id, $form_values) {
}

/**
 * Menu callback; video upload provider overview
 */
function video_upload_admin_overview() {
  $info = _video_upload_get_provider_info();
  $header = array(t('Provider'), t('Name'), t('Description'), array('data' => t('Operations'), 'colspan' => '4'),);
  $rows = array();

  // @todo add more operations and useful data field names using providers?
  foreach ($info as $provider => $info) {
      $row = array(
        check_plain($info['name']),
        check_plain($provider),
      );
      // Make the description smaller
      $row[] = array('data' => filter_xss_admin($info['description']), 'class' => 'description');
      // Set the manage column.
      $row[] = array('data' => l(t('Manage Videos'), 'admin/content/video-upload/'. $provider));

      $rows[] = $row;
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => t('No video providers available.'), 'colspan' => '7', 'class' => 'message'));
  }

  return theme('table', $header, $rows);  
}

/**
 * Video administration form

 * @todo Replace these queries with calls to the cck api, or similar, since
 * they currently break once a field has been reused in another type.
 */
function video_upload_admin_video_form(&$form_state, $provider, $options) {
  $form = array();
  // Get provider connection.
  $connection = _video_upload_connect(TRUE, $provider, $options);
  
  // pager set up
  global $pager_page_array, $pager_total;
  $page = isset($_GET['page']) ? $_GET['page'] : 0;
  $pager_page_array = explode(',', $page);
  $page++;
  $per_page = 25;


  // feed set up
  $params['page'] = $page;
  $params['per_page'] = $per_page;
  $params['search'] = false;
  $feed = _video_upload_get_feed($connection, $params, $options, $provider);

  // pager set up
  $pages = $feed['pages'];
  $pager_total[0] = $pages;
  $pager_page_array[0] =
    max(0, min(
      (int)$pager_page_array[0],
      ((int)$pager_total[0]) - 1)
    );

  if(empty($feed)) {
    return $form;
  }

  $video_ids = $all_videos = array();
  
  $fields = _video_upload_relevant_fields();
  foreach ($fields as $field => $config) {
    if ($config['provider'] == $provider) {
      $result = _video_upload_query_videos($config);
      while ($video = db_fetch_object($result)) {
        $video->field = $config;
        $video_ids[] = $video->video_id;
        $all_videos[$video->video_id] = $video;
      }    
    }
  }

  // the big feed list will be a combination of gdata objects and node objects
  // (in the event of orphaned nodes)
  $big_feed_list = $video_ids_provider = array();

  // now, with a list of ids, loop through the feed looking for
  // isolated/stranded videos that have a developer tag set by one of our
  // fields  
  foreach ($feed['videos'] as $video) {
    $id = $video->video_id;

    // add id to the provider ids (this is to find orphaned nodes later on)
    $video_ids_provider[] = $id;
    
    // set the remote flag, since this exists on the provider
    $video->remote = true;

    $big_feed_list[] = $video;
  }

  // now, loop through drupal videos to find video ids not on provider
  /* @todo - Disabled: when using paged results on large video DBs we can only
             display provider videos, or else we'll get false orphans if
             the current page does not contain some node video ids.
  */ 
  /*
  foreach ($all_videos as $id => $video) {
    if (!in_array($id, $video_ids_provider)) {
      $node = node_load(array('nid' => $video->nid));
      $video->title = $node->title;
      $video->link = l($node->title, 'node/' . $node->nid);
  
      // set the remote flag to false, since this video is not yet on the provider
      $video->remote = false;
      $big_feed_list[] = $video;
    }
  }
  */
 
  // loop through combined list of feed video and orphaned video
  foreach ($big_feed_list as $video) {
    $id = $video->video_id;
    
    // set video title
    $video->title = check_plain($video->title);
    // set link
    $video->link = l($id, theme('video_upload_video_link', array('video_provider' => $provider, 'video_id' => $id)));
    $attributes = array('width' => 128, 'height' => 72);
    $thumbnail = theme('video_upload_video_thumb', $attributes, array('video_provider' => $provider, 'video_id' => $id));
    $video->thumb = l($thumbnail, theme('video_upload_video_link', array('video_provider' => $video->provider, 'video_id' => $id)), array('html' => true, 'attributes' => array('rel' => 'video-upload-admin-preview|' . $provider . '|' . $id, 'class' => 'video-upload-admin-thumb', 'title' => $video->title)));
    
    
    $form['thumb'][$id] = array(
      '#value' => $video->thumb,
    );

    $form['id'][$id] = array(
      '#value' => $video->link,
    );

    $form['title'][$id] = array(
      '#value' => $video->title,
    );

    $form['vtitle'][$id] = array(
      '#name' => 'vtitle-' . $id,
      '#type' => 'hidden',
      '#value' => $video->title,
    );

    $node = FALSE;
    if (in_array($id, $video_ids)) {
      // this video is associated with a node, so we can fetch the node to
      // determine the status
      $nid = $all_videos[$id]->nid;

      $node = node_load(array('nid' => $nid));

      $form['node'][$id] = array('#value' => l($nid, 'node/' . $nid));

      $field_name = $all_videos[$id]->field['field_name'];
      $form['field_name'][$id] = array('#value' => l($field_name, 'admin/content/node-type/' . $node->type . '/fields'));
    }
    else {
      // Orphaned video, so we can operate on it. Videos associated with nodes
      // must be operated on through the node system.
      $videos[$id] = '';

      $form['node'][$key] = FALSE;
      $form['field_name'][$key] = FALSE;
    }

    // if the node has the status, display that
    $status = VIDEO_UPLOAD_STATUS_UNKNOWN;
    if ($node) {
      $status = $id ? $node->{$all_videos[$id]->field['field_name']}[0]['video_status'] : VIDEO_UPLOAD_STATUS_UPLOAD_PENDING;
    }
    if ($video->remote && !in_array($status, array(VIDEO_UPLOAD_STATUS_OK, VIDEO_UPLOAD_STATUS_OK_SYNCED, VIDEO_UPLOAD_STATUS_STILLFRAME))) {
      // find status from provider since it either hasn't been checked,
      // or this is an orphaned node
      
      $status = _video_upload_get_status($connection, $id, $provider);
    }
    elseif ($status != VIDEO_UPLOAD_STATUS_UPLOAD_PENDING && !$video->remote) {
      $status = VIDEO_UPLOAD_STATUS_ORPHANED;
    }
    $status_text = theme('video_upload_status_text', $status);
    $form['status'][$id] = array('#value' => $status_text);
  }

  // this constructs the checkboxes, and in fact, the only interactivity
  // currently available. If there aren't any orphaned videos, there's nothing
  // to do.
  if (!empty($videos)) {
    $form['videos'] = array(
      '#type' => 'checkboxes',
      '#options' => $videos,
    );

    $form['options'] = array(
      '#type' => 'fieldset',
      '#title' => t('Video management options'),
      '#prefix' => '<div class="container-inline">',
      '#suffix' => '</div>',
    );
    // @todo Add node create option
    $options = array();
    $options['delete'] = t('Delete selected videos');
    $options['create'] = t('Create a node with selected videos');
    if (module_exists('nodereference')) {
      $options['attach'] = t('Attach selected videos to a node');
    }

   $form['options']['operation'] = array(
      '#type' => 'select', 
      '#options' => $options,  
      '#default_value' => 'approve');
    $form['options']['submit'] = array(
      '#type' => 'submit', 
      '#value' => t('Submit'));

    // Set the provider so we can perform operations
    $form['provider'] = array(
      '#type' => 'hidden', 
      '#value' => $provider,
    );
  }

  $form['pager'] = array('#value' => theme('pager', NULL, $per_page, 0));

  return $form;
}

/**
 * @defgroup "Video Upload video processing functions."
 * @{
 */

/**
 * Upload all queued videos to the provider.
 *
 * @param array $fields
 *   CCK field definitions.
 *
 * @return integer
 *   Number of videos uploaded.
 */
function _video_upload_upload_all($fields) {
  $uploaded = 0;
  foreach ($fields as $field => $config) {
    $result = _video_upload_query_videos($config, VIDEO_UPLOAD_STATUS_UPLOAD_PENDING);
    while ($video = db_fetch_object($result)) {
      $video->field = $config;
      if (video_upload_upload($config, $video)) {
        $uploaded ++;
      }
    }
  }
  return $uploaded;
}

/**
 * Cycle through all records in a table with status of 'unknown' and attempt
 * to verify them on provider. Once verified, also attempt to update
 * title/tags on provider.
 *
 * @param $fields
 *   CCK configuration for video upload fields.
 */
function _video_upload_verify_all($fields) {
  foreach ($fields as $field => $config) {
    $video_ids = $videos = array();
    $result = _video_upload_query_videos($config, VIDEO_UPLOAD_STATUS_UNKNOWN);
    while ($video = db_fetch_object($result)) {
      $video->field = $config;
      $video_ids[] = $video->video_id;
      $videos[$video->video_id] = $video;
    }
    module_invoke("video_upload_provider_" . $config['provider'], 'verify_all', $videos);
  }
}

/**
 * Cycle through all records in a table with status of 'ok' and update
 * info on provider.
 * @param $fields
 *   CCK configuration for video upload fields.
 */
function _video_upload_update_all_videos($fields) {
  foreach ($fields as $field => $config) {
    // @todo better to collect videos into provider arrays, and invoke hook once per provider
    $video_ids = $videos = array();

    // Check all of our syncable statuses
    $statuses = array(VIDEO_UPLOAD_STATUS_OK, VIDEO_UPLOAD_STATUS_STILLFRAME);
    foreach ($statuses as $status) {
      $result = _video_upload_query_videos($config, $status);
      while ($video = db_fetch_object($result)) {
        $video->field = $config;
        $video->sync = $status;
        $video_ids[] = $video->video_id;
        $videos[$video->video_id] = $video;
      }
    }
    module_invoke("video_upload_provider_" . $config['provider'], 'update_all', $videos);
  }
}

/**
 * Cycle through all records in a table with status of 'delete' and delete
 * them if the field is configured to do so.
 *
 * @param array $fields
 *   CCK configuration for video upload fields.
 */
function _video_upload_delete_rejected_videos($fields) {
  foreach ($fields as $field => $config) {
    $video_ids = $videos = array();
    if (!$config['widget']['auto_delete_rejected_videos'] && !$config['widget']['remove_deleted_videos']) {
      continue;
    }
    $result = _video_upload_query_videos($config, VIDEO_UPLOAD_STATUS_DELETE);
    while ($video = db_fetch_object($result)) {
      $video->field = $config;    
      $video_ids[] = $video->video_id;
      $videos[$video->video_id] = $video;
    }
    module_invoke("video_upload_provider_" . $config['provider'], 'delete_rejected_videos', $videos);
  }
}

/**
 * @} End defgroup "Video Upload video processing functions."
 */

/**
 * Query to find all videos for a given field.
 *
 * @param array $config
 *   CCK field definition array.
 *
 * @param string $video_status
 *   Video status
 */
function _video_upload_query_videos($config, $status = false) {
  $db_info = $config['database_information'];

  $params[':video_id'] = $db_info['columns']['video_id']['column'];
  $params[':filefield'] = $db_info['columns']['fid']['column'];
  $params[':video_status_field'] = $db_info['columns']['video_status']['column'];
  $params[':video_status_field_2'] = $db_info['columns']['video_status']['column'];
  if ($status) {
    // We have a status, so let's poll that
    $params[':video_status_operand'] = '=';
    $params[':video_status'] = $status;
  }
  else {
    // No status provided, assuming we want everything that is not NULL
    $params[':video_status_operand'] = '<>';
    $params[':video_status'] = '';  
  }

  $multiple = $config['multiple'] ? ', t.delta' : '';
  // INNER JOIN node table to ensure we only get latest revision
  return db_query("SELECT t.%s AS video_id, t.nid, t.%s AS fid, t.%s AS video_status" . $multiple . " FROM {" . $db_info['table'] . "} t INNER JOIN {node} n ON t.vid = n.vid WHERE t.%s %s '%s'", $params); 
}

/**
 * Reset synced videos to ok status (resync)
 */
function _video_upload_resync_videos($provider) {
  $fields = _video_upload_relevant_fields();
  foreach ($fields as $field => $config) {
    if ($config['provider'] == $provider) {
      $db_info = $config['database_information'];
      $field = $db_info['columns']['video_status']['column'];
      $res = db_query("UPDATE {" . $db_info['table'] . "} t INNER JOIN {node} n ON t.vid = n.vid SET %s = '%s' WHERE %s = '%s'", $field, VIDEO_UPLOAD_STATUS_OK, $field, VIDEO_UPLOAD_STATUS_OK_SYNCED);
      if ($res) {
        drupal_set_message(t('Table %table has been updated.', array('%table' => $db_info['table'])));
      }
    }
  }
  content_flush_caches();
}
  
/**
 * Page callback for autocomplete.
 */
function video_upload_autocomplete($string = NULL) {
  $output = _video_upload_autocomplete($string);

  // let the world know this isn't normal output.
  drupal_set_header('Content-type: text/javascript');
  print drupal_to_js(drupal_map_assoc($output));
  exit;
}

function _video_upload_autocomplete($string) {
  $nodes = _video_upload_api_autocomplete($string);
  return $nodes;
}

/**
* Retrieve a pipe delimited string of autocomplete suggestions for existing users
*/
function video_upload_api_autocomplete($string) {
  $matches = array();
  
  if (empty($string)) {
    return $matches;
  }
  
  $res = db_query("SELECT type_name FROM {node_field_instance} LEFT JOIN {node_field} USING (field_name) WHERE type = '%s'", 'video_upload');
  while ($type = db_fetch_object($res)) {
    $types[] = $type->type_name;
  }

  $where = "n.type IN (" . implode(', ', array_fill(0, count($types), "'%s'")) . ')';
  $where_args = $types;

  // Run a match to see if they're specifying by nid.
  $preg_matches = array();
  $match = preg_match('/\[nid: (\d+)\]/', $string, $preg_matches);
  if (!$match) {
    $match = preg_match('/^nid: (\d+)/', $string, $preg_matches);
  }
  if ($match) {
    // If it found a nid via specification, reduce our resultset to just that nid.
    $where .= " AND n.nid = %d";
    array_push($where_args, $preg_matches[1]);
  }
  else {
    // Build the constant parts of the query.
    $where .= " AND LOWER(n.title) LIKE LOWER('%s%%')";
    array_push($where_args, $string);
  }

  $result = db_query_range(db_rewrite_sql("SELECT n.nid, n.title FROM {node} n WHERE $where"), $where_args, 0, 10);
  while ($node = db_fetch_object($result)) {
    $matches[$node->nid] = check_plain($node->title) . " [nid: $node->nid]";
  }
  
  return $matches;
}

function video_upload_preview_ajax($video_id, $provider) {
  $output = theme('video_upload_provider_' . $provider . '_video', $video_id, 640, 360);
  print drupal_to_js(array('data' => $output, 'status' => true));
  exit();  
}