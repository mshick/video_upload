<?php

// $Id: zend_gdata.inc,v 1.9.2.5.2.4 2010/07/10 01:30:52 jhedstrom Exp $

/**
 * @file Provide an abstracted API for interfacing with the Zend GData
 *   client library
 */

// Upload token url.
define('VIDEO_UPLOAD_GDATA_TOKEN_UPLOAD_URL', 'http://gdata.youtube.com/action/GetUploadToken');

// Note the different schemes for categories and developer tags.
define('VIDEO_UPLOAD_GDATA_CATEGORIES_SCHEME', 'http://gdata.youtube.com/schemas/2007/categories.cat');
define('VIDEO_UPLOAD_GDATA_DEVELOPER_TAG_SCHEME', 'http://gdata.youtube.com/schemas/2007/developertags.cat');

// Authentication url.
define('VIDEO_UPLOAD_GDATA_AUTHENTICATE_URL', 'https://www.google.com/youtube/accounts/ClientLogin');

// GData download url.
define('VIDEO_UPLOAD_ZEND_GDATA_URL', 'http://framework.zend.com/download/gdata');

// PHP version required by Gdata libraries.
define('VIDEO_UPLOAD_ZEND_GDATA_MINIMUM_PHP_VERSION', '5.1.4');

// Minimum Zend GData version.
define('VIDEO_UPLOAD_ZEND_GDATA_MINIMUM_VERSION', '1.7.2');

/**
 * Wrapper function to include necessary Zend Gdata functions and
 * libraries.
 */
function _video_upload_gdata_initialize($op) {
  _video_upload_gdata_set_include_path();
  // Include Zend loader class.
  if (@include_once('Zend/Loader.php')) {
    switch ($op) {
    case 'authenticate' :
      Zend_Loader::loadClass('Zend_Gdata_ClientLogin');
      break;
    case 'youtube' :
      Zend_Loader::loadClass('Zend_Gdata_YouTube');
      break;
    }
    return TRUE;
  }
  else {
    watchdog('video_upload', "Couldn't find the Zend client libraries.", array(), WATCHDOG_ERROR);
  }
}

/**
 * Authenticate YouTube user account.
 */
function _video_upload_gdata_authenticate_youtube() {
  // Username.
  $username = variable_get('video_upload_youtube_username', FALSE);
  if (!$username) {
    watchdog('video_upload', 'No YouTube username set', array(), WATCHDOG_ERROR);
  }

  // Password.
  $password = variable_get('video_upload_youtube_password', FALSE);
  if (!$password) {
    watchdog('video_upload', 'No YouTube password set', array(), WATCHDOG_ERROR);
  }

  if (!$username || !$password) {
    drupal_set_message(t('YouTube Uploads not currently available'), 'error');
    return FALSE;
  }

  if (_video_upload_gdata_initialize('authenticate')) {
    try {
      $http_client = Zend_Gdata_ClientLogin::getHttpClient(
        $username, // username
        $password, // password
        $service = 'youtube',
        $client = NULL,
        $source = 'drupal',
        $loginToken = NULL,
        $loginCaptcha = NULL,
        VIDEO_UPLOAD_GDATA_AUTHENTICATE_URL
      );
    }
    catch (Zend_Gdata_App_AuthException $authEx) {

      drupal_set_message(t('YouTube uploads currently unavailable (%error)', array('%error' => $authEx->getMessage())), 'error');
      watchdog('video_upload', 'Authentication error for YouTube Account (%error)', array('%error' => $authEx->getMessage()), WATCHDOG_ERROR);
      return FALSE;
    }
    catch (Zend_Gdata_App_HttpException $e) {
      drupal_set_message(t('YouTube uploads currently unavailable'), 'error');
      watchdog('video_upload', 'Authentication error for YouTube Account: %error', array('%error' => $e->getMessage()), WATCHDOG_ERROR);
      return FALSE;
    }
  }
  else {
    return FALSE;
  }

  // Pass developer key.
  $developer_key = variable_get('video_upload_youtube_developer_key', FALSE);
  if (!$developer_key) {
    watchdog('video_upload', 'No developer key set', array(), WATCHDOG_ERROR);
    drupal_set_message(t('YouTube Uploads not currently available'), 'error');
    return FALSE;
  }
  $http_client->setHeaders('X-GData-Key', 'key=' . $developer_key);

  return $http_client;
}

/**
 * Create a Zend YouTube object.
 * @param mixed $http_client
 *   Either a boolean (if FALSE, an un-authenticated connection is
 *   made), or an authentication object.
 * @return object Zend YouTube object
 */
function _video_upload_gdata_youtube($http_client = NULL) {
  _video_upload_gdata_initialize('youtube');
  try {
    $yt = new Zend_Gdata_YouTube($http_client);
    // Set API version to 2.0.
    $yt->setMajorProtocolVersion(2);
  }
  catch (Zend_Gdata_App_HttpException $e) {
    watchdog('video_upload', 'Authentication error while creating a YouTube connection object: %error', array('%error' => $e->getMessage()), WATCHDOG_ERROR);
  }
  return $yt;
}

/**
 * Create a Zend YouTube VideoEntry object.
 * @return object Zend YouTube VideoEntry object
 */
function _video_upload_gdata_video_entry() {
  _video_upload_gdata_initialize('youtube');
  $ytv = new Zend_Gdata_YouTube_VideoEntry();
  // Set API version to 2.0.
  $ytv->setMajorProtocolVersion(2);
  return $ytv;
}

/**
 * Get a YouTube Token Array.
 * @param object $yt
 *   Zend YouTube object.
 * @param object $video
 *   A VideoEntry object.
 * @return array
 *   Token Array.
 */
function _video_upload_gdata_get_token_array($yt, $video) {
  $token_array = FALSE;

  try {
    $token_array = $yt->getFormUploadToken($video, VIDEO_UPLOAD_GDATA_TOKEN_UPLOAD_URL);
  }
  catch (Zend_Gdata_App_HttpException $http_ex) {
    // Here, we'll try one more time.
    sleep(1);
    try {
      $token_array = $yt->getFormUploadToken($video, VIDEO_UPLOAD_GDATA_TOKEN_UPLOAD_URL);
    }
    catch (Zend_Gdata_App_HttpException $http_ex) {
      drupal_set_message(t('Video uploads currently unavailable. If this problem persists, contact the system administrator.'), 'error');
      watchdog('video_upload', 'YouTube Connection failed with @error', array('@error' => $http_ex->getMessage()), WATCHDOG_ERROR);
      return FALSE;
    }
  }
  return $token_array;
}

/**
 * Construct a media group.
 * @param object $yt
 *   Zend GData YouTube object.
 * @param object $video
 *   Zend GData VideoEntry object.
 * @param object $node
 *   Node object.
 * @param array $field
 *   CCK Field Definition.
 */
function _video_upload_gdata_construct_media_group(&$yt, &$video, $node, $field) {
  $widget = $field['widget'];
  
  $mediaGroup = $yt->newMediaGroup();

  // Set the title.
  $yt_title = _video_upload_set_default_title($node, $widget, 'youtube');
  $mediaGroup->title = $yt->newMediaTitle()->setText($yt_title);

  // Set the description.
  $yt_description = _video_upload_set_default_description($node, $widget, 'youtube');
  $mediaGroup->description = $yt->newMediaDescription()->setText($yt_description);

  // Set the video category.
  // @fixme there is a descrepency between the available categories
  //   listed in the category scheme, and what YouTube actually
  //   accepts. For now, until this can be resolved, "Entertainment"
  //   is sent for everything.
  $_yt_category = 'Entertainment'; //_video_upload_get_video_category($node, $field);
  $yt_category = $yt->newMediaCategory()->setText($_yt_category)->setScheme(VIDEO_UPLOAD_GDATA_CATEGORIES_SCHEME);

  // Set the developer tags.
  $_yt_dev_tags = _video_upload_provider_youtube_set_developer_tags($node, $widget);
  foreach ($_yt_dev_tags as $dev_tag) {
    $yt_dev_tags[] = $yt->newMediaCategory()->setText($dev_tag)->setScheme(VIDEO_UPLOAD_GDATA_DEVELOPER_TAG_SCHEME);
  }

  // Construct media group category array.
  $mediaGroup->category = array_merge(array($yt_category), $yt_dev_tags);

  // Set the public tags.
  $yt_keywords = _video_upload_get_video_keywords($node, $widget, 'youtube');
  $mediaGroup->keywords = $yt->newMediaKeywords()->setText($yt_keywords);

  if ($widget['publish_sync']) {
    $list = $node->status ? 'allowed' : 'denied';
    // Zend framework does not natively support yt:accessControl, so we build it here
    $accessControlElement = new Zend_Gdata_App_Extension_Element('yt:accessControl', 'yt', 'http://gdata.youtube.com/schemas/2007', '');
    $accessControlElement->extensionAttributes = array(
      array('namespaceUri' => '', 
            'name' => 'action', 
            'value' => 'list'), 
      array('namespaceUri' => '', 
            'name' => 'permission', 
            'value' => $list)); 
    $video->extensionElements = array($accessControlElement); 
  }

  $video->mediaGroup = $mediaGroup;
}

/**
 * Modeled upon hook_requirements(), called from
 * video_upload_requirements().
 */
function _video_upload_gdata_requirements($phase) {
  _video_upload_gdata_set_include_path();

  $requirements = array();
  $t = get_t();

  // some boilerplate install text
  $install_details = $t('Information on installing the required library can be found <a href="!url">here</a>.', array('!url' => url('http://code.google.com/support/bin/answer.py?answer=76585')));


  if (version_compare(PHP_VERSION, VIDEO_UPLOAD_ZEND_GDATA_MINIMUM_PHP_VERSION, '<')) {
    $requirements['zend_loader'] = array(
      'title' => $t('Zend GData Library: PHP Version'),
      'value' => $t('Too low'),
      'description' => $t('The Zend Client library requires a PHP version of !zversion or higher, and this machine is running PHP version !phpversion', array('!zversion' => VIDEO_UPLOAD_ZEND_GDATA_MINIMUM_PHP_VERSION, '!phpversion' => PHP_VERSION)),
      'severity' => REQUIREMENT_ERROR,
    );
  }
  elseif (!include_once('Zend/Loader.php')) {
    $requirements['zend_loader'] = array(
      'title' => $t('Zend Loader'),
      'value' => $t('Not found'),
      'severity' => REQUIREMENT_ERROR,
      'description' => $t('The Zend Client library loader is required by the Video Upload module, and has not been found. This is a strong indication that the required Zend Gdata libraries are not installed. These can be downloaded  <a href="!url">here</a>.<br /><br />!install', array('!url' => url(VIDEO_UPLOAD_ZEND_GDATA_URL), '!install' => $install_details)),
    );
  }
  elseif (!@include_once('Zend/Gdata.php')) {
    $requirements['zend_gdata'] = array(
      'title' => $t('Zend GData Library'),
      'value' => $t('Not Found'),
      'severity' => REQUIREMENT_ERROR,
      'description' => $t('The Zend Client library loader was found, but the GData library required by the Video Upload module was not. This can be downloaded  <a href="!url">here</a>.<br /><br />!install', array('!url' => url(VIDEO_UPLOAD_ZEND_GDATA_URL), '!install' => $install_details)),
    );
  }
  elseif (version_compare(Zend_Version::VERSION, VIDEO_UPLOAD_ZEND_GDATA_MINIMUM_VERSION, '<')) {
    // Check version.
    $requirements['zend_gdata'] = array(
      'title' => $t('Zend GData Library'),
      'value' => $t('Version too low'),
      'severity' => REQUIREMENT_WARNING,
      'description' => $t('The version of the Zend GData library installed (%installed) is lower than the recommended version (%recommended). Some features may not work properly. A newer version of the library can be downloaded <a href="!url">here</a>.', array('%installed' => Zend_Version::VERSION, '%recommended' => VIDEO_UPLOAD_ZEND_GDATA_MINIMUM_VERSION, '!url' => url(VIDEO_UPLOAD_ZEND_GDATA_URL))),
    );
  }


  return $requirements;
}

/**
 * Set include path for Zend GData client libraries, which should be
 * installed locally in the module directory.
 */
function _video_upload_gdata_set_include_path() {
  // Include path must be modified.
  static $path;
  if (!$path) {
    $path = get_include_path();
    set_include_path($path . PATH_SEPARATOR . drupal_get_path('module', 'video_upload') . '/providers/youtube');
  }
}


/**
 * Check the status of a video object.
 * @param object $video
 *   Zend GData VideoEntry object.
 * @return object
 */
function _video_upload_gdata_get_video_status($video) {
  $status = new stdClass();
  try {
    $control = $video->getControl();
  }
  catch (Zend_Gdata_App_Exception $e) {
    return;
  }

  if ($control instanceof Zend_Gdata_App_Extension_Control) { 
    if ($control->getDraft() != NULL && $control->getDraft()->getText() == 'yes') {
      $state = $video->getVideoState();
      if ($state instanceof Zend_Gdata_YouTube_Extension_State) {
        switch ($state->getName()) {
          case 'rejected' :
            $status->status = FALSE;
            $status->message = $state->getText();
            return $status;
          case 'processing' :
            // Can't determine final state if processing.
          default:
            return;
        }
      }
      else {
        // @FIXME not sure what this means either
      }
    }
  }
  else {
    try {
      // @TODO Find a way to check the state other than
      //       grabbing a title.
      $state = $video->getVideoTitle();
    }
    catch (Zend_Gdata_App_Exception $e) {
      // @TODO A useful error to watchdog.
    }
    if ($state) {
      $status->status = TRUE;
    }
  }
  return $status;
}

/**
 * Get specified atom feed.
 * @param string $uri
 *   URI for the feed.
 * @return array
 *   An array of Zend Gdata video objects.
 */
function _video_upload_gdata_get_feed($uri, $yt) {
  if (!$yt) {
    return FALSE;
  }

  try {
    $feed = $yt->getVideoFeed($uri);
  }
  catch (Zend_Gdata_App_HttpException $e) {
    watchdog('video_upload', 'Video Upload: error getting video feed: @error', array('@error' => $e->getMessage()), WATCHDOG_ERROR);
    return FALSE;
  }
  return $feed;
}

/**
 * Delete a video from YouTube.
 */
function _video_upload_gdata_delete_video($yt, $video) {
  try {
    $result = $yt->delete($video);
    watchdog('video_upload', 'Video Upload: Video %id has been deleted from YouTube', array('%id' => $video->getVideoId()));
    // @todo return some useful feedback
  }
  catch (Zend_Gdata_App_HttpException $e) {
    watchdog('video_upload', 'Video Upload: error %error while attempting to delete video id @id', array('%error' => $e->getMessage(), '@id' => $video->getVideoId()), WATCHDOG_ERROR);
  }
}

/**
 * Update a video.
 */
function _video_upload_gdata_update_video($yt, $video) {
  $url = $video->getEditLink()->getHref();
  try {
    $yt->updateEntry($video, $url);
    return TRUE;
  }
  catch (Zend_Gdata_App_HttpException $e) {
    watchdog('video_upload', 'Video Upload: error %error while attempting to update video id @id', array('%error' => $e->getMessage(), '@id' => $video->getVideoId()), WATCHDOG_ERROR);
    return FALSE;
  }
}


/**
 * Determine if a video is a Zend GData object instance
 */
function _video_upload_is_zend_object($video) {
  return $video instanceof Zend_Gdata_YouTube_VideoEntry;
}

/**
 * Attach file to video object.
 * @param object $yt
 * @param object $yt_video
 * @param string $filepath
 * @param string $mimetype
 */
function _video_upload_gdata_attach_video(&$yt, &$yt_video, $filepath, $mimetype) {
  // Create a new Zend_Gdata_App_MediaFileSource object.
  $filesource = $yt->newMediaFileSource($filepath);
  $filesource->setContentType($mimetype);
  // Set slug header.
  $filesource->setSlug($filepath);

  // Add the filesource to the video entry.
  $yt_video->setMediaSource($filesource);
}

/**
 * Perform actual upload.
 */
function _video_upload_gdata_insert_video(&$yt, &$yt_video) {
  // Try to upload the video, catching a Zend_Gdata_App_HttpException
  // if available or just a regular Zend_Gdata_App_Exception.
  try {
    $new_video = $yt->insertEntry($yt_video, VIDEO_UPLOAD_YOUTUBE_UPLOAD_URL, 'Zend_Gdata_YouTube_VideoEntry');
    // Set API version to 2.0.
    $new_video->setMajorProtocolVersion(2);

    if ($new_video) {
      // Translate zend result back into Drupal/FileField.
      $result['video_id'] = $new_video->getVideoId();
    }
    return $result;
  } catch (Zend_Gdata_App_HttpException $e) {
    watchdog('video_upload', 'Video Upload: error %error while attempting to upload a video.', array('%error' => $e->getRawResponseBody()));
  } catch (Zend_Gdata_App_Exception $e) {
    watchdog('video_upload', 'Video Upload: error %error while attempting to upload a video.', array('%error' => $e->getMessage()));
  }
}