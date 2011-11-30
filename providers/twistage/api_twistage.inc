<?php

/**
 * @file Provide an abstracted API for interfacing with Twistage
 */

// API BASE URL
define('VIDEO_UPLOAD_TWISTAGE_API_BASE', 'http://service.twistage.com');


/**
 * Wrapper function to include necessary Twistage Video API
 * libraries
 */
function _video_upload_twistage_api_initialize() {
  _video_upload_twistage_api_set_include_path();
  // Include Twistage video_api classes
  if (@include_once('video_api/VideoApi.php')) {
    return true;
  }
  else {
    watchdog('video_upload', 'Couldn\'t find the Twistage client libraries', WATCHDOG_ERROR);
  }
}

/**
 * Set include path for Twistage Video API client libraries, which should be
 * installed locally in the module directory.
 */
function _video_upload_twistage_api_set_include_path() {
  // include path must be modified
  static $path;
  if (!$path) {
    $path = get_include_path();
    set_include_path($path . PATH_SEPARATOR . drupal_get_path('module', 'video_upload_twistage'));
  }
}

/**
 * Create a new Twistage VideoAPI object
 * @param string $library_id is the optional library_id to specify for scoping.
 * @return object $twistage is a Twistage VideoAPI object
 */
function _video_upload_twistage_api_twistage($library_id = false) {
  // username
  $license_key = variable_get('video_upload_twistage_license_key', false);
  if (!$license_key) {
    watchdog('video_upload', t('No Twistage license key set'), WATCHDOG_ERROR);
  }
  $library_id = $library_id ? $library_id : variable_get('video_upload_twistage_library_id', false);
  if (!$library_id) {
    watchdog('video_upload', t('No Twistage library id set'), WATCHDOG_ERROR);
  }
  $account_id = variable_get('video_upload_twistage_company', false);  
  if (!$account_id) {
    watchdog('video_upload', t('No Twistage company set'), WATCHDOG_ERROR);
  }

  if (_video_upload_twistage_api_initialize()) {
    try {
      $twistage = VideoApi::for_library ( VIDEO_UPLOAD_TWISTAGE_API_BASE,
                                    $account_id,
                                    $library_id,
                                    $license_key
                    );
      return $twistage;
    }
    catch (Exception $e) {
      drupal_set_message(t('Twistage uploads currently unavailable'), 'error');
      watchdog('video_upload', t('General Twistage API exception.'), WATCHDOG_ERROR);
      return false;   
    }
  }
  else {
    return false;
  }
}

/**
 * Twistage authentication
 * @param object $twistage is a videoApi object.
 * @param string $contributor is an email address to authenticate as for ingest.
 * @param string $op is the type of authentication to perform-options are "ingest", "view", or "update"
 * @return string $signature is a Twistage authentication signature
 */
function _video_upload_twistage_api_authenticate($twistage, $contributor, $op = false) {
  try {
    switch ($op) {
      case 'ingest':
        $contributor = $contributor ? $contributor : variable_get('video_upload_twistage_contributor', false);
        if (!$contributor) {
          drupal_set_message(t('Twistage uploads currently unavailable'), 'error');        
          watchdog('video_upload', t('No Twistage contributor set for ingest'), WATCHDOG_ERROR);
          return false;
        }
        return $twistage->authenticateForIngest($contributor);

      case 'view':
        return $twistage->authenticateForView();
              
      case 'update':
        return $twistage->authenticateForUpdate();

    }
  }
  catch (Exception $e) {
    drupal_set_message(t('Twistage uploads currently unavailable'), 'error');
    watchdog('video_upload', t('General Twistage API exception.'), WATCHDOG_ERROR);
    return false;   
  }  
}

/**
 * Update a video
 * @param string $id is the video id to operate on.
 * @param object $twistage is a videoApi object.
 * @param array $update is an array of the items to update
 * @return true if the video is updated, and false if there is an error.
 */
function _video_upload_twistage_api_update_video($id, $twistage, $update) {
  try {
    $twistage->updateVideo($id, $update);
    return true;
  } 
  catch (Exception $e) {
    drupal_set_message(t('Twistage uploads currently unavailable'), 'error');
    watchdog('video_upload', t('General Twistage API exception.'), WATCHDOG_ERROR);
    return false;   
  }
}

/**
 * Update a video state
 * @param string $id is the video id to operate on.
 * @param object $twistage is a videoApi object.
 * @param boolean $visible is the visibility state
 * @return true if the video is updated, and false if there is an error.
 */
function _video_upload_twistage_api_set_video_visible($id, $twistage, $visible) {
  try {
    $twistage->setVideoVisible($id, $visible);
    return true;
  }
  catch (Exception $e) {
    drupal_set_message(t('Twistage uploads currently unavailable'), 'error');
    watchdog('video_upload', t('General Twistage API exception.'), WATCHDOG_ERROR);
    return false;   
  }
}

/**
 * Get video metadata object
 * @param string $id is the video id to operate on.
 * @param object $twistage is a videoApi object.
 * @return object $video is the video's metadata as a json object.
 */
function _video_upload_twistage_api_get_metadata($id, $twistage) {
  try {
    $video = $twistage->getVideoMetadata($id);
    return $video;
  }
  catch (Exception $e) {
    drupal_set_message(t('Twistage uploads currently unavailable'), 'error');
    watchdog('video_upload', t('General Twistage API exception.'), WATCHDOG_ERROR);
    return false;   
  }
}

/**
 * Deletes the Twistage video
 * @param string $id is the video id to operate on.
 * @param object $twistage is a videoApi object.
 * @return true if the video is deleted, and false if there's an error
 */
function _video_upload_twistage_api_delete_video($twistage, $video) {
  try {
    $twistage->deleteVideo($video->video_id);
    return true;
  }
  catch (Exception $e) {
    drupal_set_message(t('Twistage uploads currently unavailable'), 'error');
    watchdog('video_upload', t('General Twistage API exception.'), WATCHDOG_ERROR);
    return false;   
  }
}

/**
 * Twistage video search
 * @param object $twistage is a videoApi object.
 * @param int $page is a page of results to return.
 * @param string $search is a search term to look for.
 * @return json object of the search results
 */
function _video_upload_twistage_api_search_videos($twistage, $page = 1, $per_page = 25, $search = NULL) {
  $critera = array();
  $criteria['verbosity'] = 'low';
  $criteria['hidden'] = 'false';
  $criteria['direction'] = 'down';
  $criteria['per_page'] = $per_page;
  $criteria['page'] = $page;
  $criteria['sort'] = 'created_at';
/*   if(isset($search)) { $criteria['query'] = $search; } */
  
  try {
    return $twistage->searchVideos($criteria);
  }
  catch (Exception $e) {
    drupal_set_message(t('Twistage uploads currently unavailable'), 'error');
    watchdog('video_upload', t('General Twistage API exception.'), WATCHDOG_ERROR);
    return false;   
  }  
}

/**
 * Modeled upon hook_requirements(), called from
 * video_upload_twistage_requirements()
 */
function _video_upload_twistage_api_requirements($phase) {
  _video_upload_twistage_api_set_include_path();
  $requirements = array();
  $t = get_t();

  if ($phase == 'runtime') {
    // Gdata download url
    define('VIDEO_UPLOAD_TWISTAGE_API_URL', 'http://console.twistage.com/doc/code/php/video_api.zip');

    // php version required by Gdata libraries
    define('VIDEO_UPLOAD_TWISTAGE_API_MINIMUM_PHP_VERSION', '5.2');

    // some boilerplate install text
    $install_details = $t('Information on installing the required library can be found <a href="!url">here</a>.', array('!url' => url('http://console.twistage.com/doc/code/php/library')));

    if (PHP_VERSION < VIDEO_UPLOAD_TWISTAGE_API_MINIMUM_PHP_VERSION ) {
      $requirements['twistage_videoapi'] = array(
        'title' => $t('Twistage API Library: PHP Version'),
        'value' => $t('Too low'),
        'description' => $t('The Twistage VideoAPI library requires a PHP version of !zversion or higher, and this machine is running PHP version !phpversion', array('!zversion' => VIDEO_UPLOAD_TWISTAGE_API_MINIMUM_PHP_VERSION, '!phpversion' => PHP_VERSION)),
        'severity' => REQUIREMENT_ERROR,
      );
    }
    elseif (!@include_once('video_api/VideoApi.php')) {
      $requirements['twistage_videoapi'] = array(
        'title' => $t('Twistage VideoAPI'),
        'value' => $t('Not found'),
        'severity' => REQUIREMENT_ERROR,
        'description' => $t('The Twistage VideoAPI Client library is required by the Video Upload module, and has not been found. This is a strong indication that the required Twistage VideoAPI libraries are not installed. These can be downloaded  <a href="!url">here</a>.<br /><br />!install', array('!url' => url(VIDEO_UPLOAD_TWISTAGE_API_URL), '!install' => $install_details)),
      );
    }
  }
  return $requirements;
}

function _video_upload_twistage_api_insert_video($twistage, $video){
  try {
    $video_id = $twistage->uploadVideo($video->filepath, $video->contributor, $video->update);
    return $video_id;
  }
  catch (Exception $e) {
    drupal_set_message(t('Twistage uploads currently unavailable'), 'error');
    watchdog('video_upload', t('General Twistage API exception.'), WATCHDOG_ERROR);
    return false;   
  }
}