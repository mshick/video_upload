<?php

require_once ('HttpClientCurl.php');

class VideoApi {

  private $baseUrl;
  private $libraryId;
  private $licenseKey;
  private $accountId;
  private $rssAuthRequired;
  private $http;

  private $viewToken;
  private $updateToken;
  private $ingestToken;

  private $authTokenDurationInMinutes;

  /**
   * Creates a VideoApi object scoped to the given library
   * within the given account.
   * 
   * @param string $baseUrl the service base url - see online documentation for this value.
   * @param string $accountId the account's ID
   * @param string $libraryId the ID of the library within the account to work with
   * @param string $licenseKey the license key to use for all authorization requests.  it can be the license key of a user associated with the given library, or an account-wide user.
   * @return VideoApi the created VideoApi object
  */
  public static function for_library ($baseUrl, $accountId, $libraryId, $licenseKey) {
    
    if (! $libraryId) {
      throw new VideoApiException ("VideoApi.for_library: library_id required.");
    }

    return new VideoApi ($baseUrl, $accountId, $libraryId, $licenseKey);
  }

  /**
   * Creates a VideoApi object scoped to the entire account (i.e. not to a specific library within the account).
   * 
   * @param string $baseUrl the service base url - see online documentation for this value.
   * @param string $accountId the account's ID
   * @param string $licenseKey the license key to use for all authorization requests.  it must be the license key for an account-level user, not a user assigned to a specific library.
   * @return VideoApi the created VideoApi object
   *
   * Note: to call the video ingest or import methods, you must
   * call VideoApi.for_library instead, or those methods will
   * raise an error.
  */
  public static function for_account ($baseUrl, $accountId, $licenseKey) {
    return new VideoApi ($baseUrl, $accountId, null, $licenseKey);
  }

  /**
   * Takes the filepath of a PHP .ini file and parses it for
   * the following values: base_url, account_id, library_id, license_key.
   * library_id is optional.
   * 
   * @return VideoApi object initialized with the values in the ini file.
   */
  public static function from_settings_file ($filepath) {
    return VideoApi::from_props (parse_ini_file ($filepath));    
  }

  /**
   * @param array $conf a hash containing the following keys: base_url, 
   * account_id, license_key, and optionally library_id.
   * @return VideoApi object initialized with the values in the hash.
   */
  public static function from_props ($conf) {
    if (isset($conf['library_id'])) {
      return VideoApi::for_library ($conf['base_url'],
				    $conf['account_id'],
				    $conf['library_id'],
				    $conf['license_key']);
    } else {
      return VideoApi::for_account ($conf['base_url'],
				    $conf['account_id'],
				    $conf['license_key']);
    }
  }

  /**   
   * Calls the authentication API.
   * 
   * @return string a one-time-use read-authentication signature.
   * 
   * In general you do not need to call this directly - the other methods in this class
   * that require an authentication signature will call this for you.
  */
  public function authenticateForView () {
    return $this->getAuthSignature ($this->getViewAuthToken(), 
				    $this->authTokenDurationInMinutes);
  }

  /**
   * 
   * Calls the authentication API.
   * 
   * @return string a one-time-use update-authentication signature.
   * 
   * In general you do not need to call this directly - the other methods in this class
   * that require an authentication signature will call this for you.
  */
  public function authenticateForUpdate () {	
      return $this->getAuthSignature ($this->getUpdateAuthToken(), 
				      $this->authTokenDurationInMinutes);
  }

  /**
   * Calls the authentication API.
   * 
   * @return string a one-time-use ingest-authentication signature.
   * 
   * In general you do not need to call this directly - the other methods in this class
   * that require an authentication signature will call this for you.
   *
   * @param contributor the string to use as the video's contributor
   * @param params a hash containing other params to include.  currently the
   * only param to include is :ingest_profile, specifying the ingest profile 
   * to use when ingesting the video.
   * 
  */
  public function authenticateForIngest ($contributor, $params=null) {
    $params = ($params ? $params : array());

    if (! $contributor) {
      throw new VideoApiException ("You must provide a non-blank contributor to obtain an ingest authentication signature.");
    }

    if (! $this->getLibraryId()) {
      throw new VideoApiException ("You must provide a non-blank library ID to obtain an ingest authentication signature.");
    }

    return $this->getAuthSignature ($this->getIngestAuthToken(),
				    0,
				    array_merge ($params,
						 array('userID' => $contributor,
						       'library_id' => $this->getLibraryId())));
  }

  public function isViewTokenExpired() {
    return $this->getViewAuthToken()->isExpired ();
  }

  public function isUpdateTokenExpired() {
    return $this->getUpdateAuthToken()->isExpired ();
  }
    
  public function resetAuthTokenCache() {
    $this->getViewAuthToken()->resetCache();
    $this->getUpdateAuthToken()->resetCache();
  }

  /**
   * Sets the auth token duration for subsequent calls to the authentication API.
   * The default value is 15 minutes - call this method to change it.  To force
   * a call for a new signature every time one is needed, call this with 0 or
   * call clearAuthTokenDuration().
   */
  public function setAuthTokenDurationInMinutes($durationInMinutes) {
    $this->authTokenDurationInMinutes = $durationInMinutes;
  }

  /**
   * Returns the duration in minutes that will be provided in subsequent calls
   * to the authentication API, requesting tokens that last that long.
   */
  public function getAuthTokenDurationInMinutes() {
    return $this->authTokenDurationInMinutes;
  }

  /**
   * Sets the auth token duration parameter value to 0, forcing a call to
   * the authentication API every time a signature is needed.
   */
  public function clearAuthTokenDuration() {
    $this->setAuthTokenDurationInMinutes (0);
  }

  /**
   * Returns HTML for the upload wizard, generating the
   * ingest authenticatin token
   * for the given contributor, and encoding the given $params
   * PHP associative array into the equivalent JSON hash.
   * 
   * @param params a PHP associative array containing the same configuration
   * parameters you would construct the JSON object from to initialize the upload
   * wizard, as described in the online documentation.
   * 
   * example:
   * 
   * echo $api->createUploadWizard("admin", "http://yoursitename.com/redirect/url",
   * array ("ingest_profile" => "development",
   * "custom_id" => "12345"));
   * 
   */
  public function createIngestWizard($contributor, $redirectUrl, $params=null) {

    $auth = $this->authenticateForIngest ($contributor);
    
    $thirdParam = ($params ? ", " . json_encode($params) : "");

    return <<<DOC
      <script type="text/javascript" src="http://service.twistage.com/api/publish_script"></script>
      <script>
        uploadWizard('$auth', '$redirectUrl' $thirdParam);
      </script>
DOC;
  }

  /**
   * @param videoId the video ID
   * @param selector specifies how to interpret the value parameter.  can be one of "asset_id", "format", "ext" (for extension).
   * @return string the specified video asset's download URL
   * If selector is null, returns the download url for the main asset.
   * 
   * For example, to get the URL for a file's main asset,
   * 
   * $url = $api->getDownloadUrl ($videoId);
   * 
   * to get the URL of a file by asset_id
   * 
   * $url = $api->getDownloadUrl ($videoId, 'asset_id', '12345');
   * 
   * to get the URL of a file by format name
   * 
   * $url = $api->getDownloadUrl ($videoId, 'format', 'ipod');
   * 
   * to get the URL of a file by extension
   * 
   * $url = $api->getDownloadUrl ($videoId, 'ext', 'flv');
   * 
   * to get the URL of a file's source asset:
   * 
   * $url = $api->getDownloadUrl ($videoId, 'ext', 'source');
  */
  public function getDownloadUrl ($videoId, $selector=null, $value=null) {

    $subUrl = "videos/$videoId/";

    if ($selector == 'asset_id') {
      $subUrl .= "assets/$value/file";
    }

    else if ($selector == 'format') {
      $subUrl .= "formats/$value/file";
    }

    else if ($selector == 'ext') {
      $subUrl .= "file.$value";
    }

    else if ($selector == null) {
      $subUrl .= "file";
    }

    else {
      throw new VideoApiException ("Unrecognized selector: $selector");
    }

    return $this->createUrl ($subUrl);
  }

  /**
   * @return string the download URL for the given video's source asset.
   */
  public function getDownloadUrlForSourceAsset ($videoId) {
    return $this->getDownloadUrl ($videoId, 'ext', 'source');
  }
  
  /**
   * returns the URL of the given video's stillframe image with the specified width.
   * If no width specified, defaults to 50.
   * 
   * Currently width can only be one of the values 50, 150 and 320.
   * 
  */
  public function getStillFrameUrl ($videoId, $params=null) {    

    $params = ($params ? $params : array());

    $width = isset($params['width']) ? $params['width'] : null;
    $height = isset($params['height']) ? $params['height'] : null;

    return $this->createUrl ("videos/$videoId/screenshots/" . 
			     ($width || $height ? "" : 'original') .
			     ($width ? "${width}w" : '') . 
			     ($height ? "${height}h" : '') . 
			     ".jpg");
  }
  
  /**
   * Calls the Video Metadata API.
   * 
   * @param $videoId
   * @param $format either 'xml' or 'json'.  if provided, returns the string result in that format.  if not provided, returns the result of parsing the json result into a PHP object tree.
   * 
  */
  public function getVideoMetadata ($videoId, $format=null) {

    if ($format == null) {
      return $this->cleanupItemCustomFields(json_decode ($this->getVideoMetadata ($videoId, 'json')));
    } else {
      try {
	return $this->getHttp()->get ($this->createUrl ("videos/$videoId.$format"),
				      $this->addViewAuthParam());
      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }
    }
  }

  /**
   * Calls the Track Metadata API.
   * 
   * @param $trackId
   * @param $format either 'xml' or 'json'.  if provided, returns the string result in that format.  if not provided, returns the result of parsing the json result into a PHP object tree.
   * 
   */
  public function getTrackMetadata ($trackId, $format=null) {

    if ($format == null) {
      return $this->cleanupItemCustomFields(json_decode ($this->getTrackMetadata ($trackId, 'json')));
    } else {
      try {
	return $this->getHttp()->get ($this->createUrl ("tracks/$trackId.$format"),
				      $this->addViewAuthParam());
      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }
    }
  }

  /**
   * Calls the Playlist Metadata API.
   * 
   * @param $videoId
   * @param $format either 'rss' or 'json'.  if provided, returns the string result in that format.  if not provided, returns the result of parsing the json result into PHP objects (return an array of video objects).
   * 
  */
  public function getPlaylistMetadata ($playlistId, $format=null) {
    if ($format == null) {
      return json_decode ($this->getPlaylistMetadata ($playlistId, 'json'));
    } else {
      try {
	return $this->getHttp()->get ($this->createUrl ("playlists/$playlistId.$format"),
				      $this->addViewAuthParam());
      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }
    }
  }
  
  /**
   * Returns the URL of a video RSS feed for all videos matching the
   * given criteria.
   * 
   * If this api object was created using for_library, the
   * RSS feed is restricted to the library.  If for_account was used,
   * the results are from the entire account.
   * 
   * See the online documentation for details of the accepted criteria.
  */
  public function getRssUrl ($criteria) {
    return $this->createVideoSearchUrl ($criteria, 'rss');
  }

  /**
   * Calls the Search Videos API, scoping to the account or library
   * depending on whether this api object was constructed using
   * for_account or for_library.
   * 
   * @param format optional format type.  if omitted, this method returns the metadata as a tree of php objects, generated by obtaining the search results in json format and parsing it.
   * 'xml' returns the search results as an xml string.
   * 'json' returns the search results as a json string.
   * 
   * See the online documentation for the "Search Videos" API for details
   * of accepted search criteria.
  */
  public function searchVideos ($criteria, $format=null) {

    if ($format == null) {
      
      $page = json_decode ($this->searchVideos ($criteria, 'json'));

      $this->cleanupSearchResults ($page->videos);      

      return $page;

    } else {
      try {

	return $this->getHttp()->get ($this->createVideoSearchUrl($criteria, 
								  $format));

      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }      
    }
  }

  /**
   * Returns the URL of an audio track RSS feed for all tracks matching the
   * given criteria.
   * 
   * If this api object was created using for_library, the
   * RSS feed is restricted to the library.  If for_account was used,
   * the results are from the entire account.
   * 
   * See the online documentation for details of the accepted criteria.
  */
  public function getTracksRssUrl ($criteria) {
    return $this->createTrackSearchUrl ($criteria, 'rss');
  }

  /**
   * Calls the Audio Tracks Search API, scoping to the account or library
   * depending on whether this api object was constructed using
   * for_account or for_library.
   * 
   * @param format optional format type.  if omitted, this method returns the metadata as a tree of php objects, generated by obtaining the search results in json format and parsing it.
   * 'xml' returns the search results as an xml string.
   * 'json' returns the search results as a json string.
   * 
   * See the online documentation for details
   * of accepted search criteria.
  */
  public function searchTracks ($criteria, $format=null) {

    if ($format == null) {
      
      $page = json_decode ($this->searchTracks ($criteria, 'json'));

      $this->cleanupSearchResults ($page->tracks);

      return $page;

    } else {
      try {
	return $this->getHttp()->get ($this->createTrackSearchUrl($criteria, 
								  $format));
      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }      
    }
  }

  /**          
   * Calls the Video Tags API, returning all video IDs tagged with the
   * given tag.  Scopes the request to the whole account
   * or to a specific library depending on whether you created
   * this api object with for_account or for_library.
   * 
   * @param format optional format - 'xml', 'json', or null.  If
   * null, returns the result of parsing the json results into
   * PHP objects.
  */
  public function getVideosWithTag ($tag, $format=null) {
    if ($format == null) {
      return $this->collectVideoIdsFromVideos (json_decode ($this->getVideosWithTag($tag, 'json')));
    } else {
      try {
	return $this->getHttp()->get ($this->createScopedUrl("tags/$tag.$format"),
				      $this->addViewAuthParam());
      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }
    }
  }

  /**
   * Calls the Video Tags API, returning a list of all tags
   * and the number of videos with each tag.
   * 
   * Scopes the request
   * to the whole account if you created this api object using for_account,
   * and to the specific library if you created this api object with for_library.
   * 
   * @param format 'xml', 'json' or null.  If null, returns the result of
   * parsing the json results into PHP objects.
  */
  public function getTags($format=null) {
    if ($format == null) {
      return json_decode ($this->getTags ('json'));
    } else {
      try {
	return $this->getHttp()->get ($this->createScopedUrl("tags.$format"),
				      $this->addViewAuthParam());
      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }
    }
  }

  /**
   * Returns a list of tags (just their names, not the number of videos
   * using each tag).
   * 
   * Scopes the request
   * to the whole account if you created this api object using for_account,
   * and to the specific library if you created this api object with for_library.
   */
  public function getTagNames () {
    $result = $this->getTags();
    $names = array();
    foreach ($result as $tag) {
      $names[] = $tag->name;
    }
    return $names;
  }

  
  /**
   * Calls the Video Delivery Statistics API.
   * 
   * @param params The parameters to apply to the API request.
   * @param format 'json', 'xml', 'csv', or null.  If null, returns the
   * result of parsing the json results into PHP objects.
   * 
   * See the online documentation for the accepted params and the
   * return data structure.
  */
  public function getDeliveryStats ($params, $format=null) {
    if ($format == null) {
      return json_decode ($this->getDeliveryStats ($params, 'json'));
    } else {
      try {
	return $this->getHttp()->get ($this->createScopedUrl("statistics/video_delivery.$format"), 
				      $this->addViewAuthParam ($params));
      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }
    }
  }
    
  /**
   * Calls the Video Delivery Statistics API for the specified video.
   * 
   * @param params The parameters to apply to the API request.
   * @param format 'json', 'xml', 'csv', or null.  If null, returns the
   * result of parsing the json results into PHP objects.
   * 
   * @return array|string the usage statistics data for the given video, constrained usage the given
   * criteria, as a List of VideoStats objects.
   * 
   * See the online documentation for details of the accepted params.
  */
  public function getDeliveryStatsForVideo ($videoId, $params, $format=null) {
    if ($format == null) {
      return json_decode ($this->getDeliveryStatsForVideo ($videoId, $params, 'json'));
    } else {
      try {
	return $this->getHttp()->get ($this->createUrl ("videos/$videoId/statistics.$format"),
				      $this->addViewAuthParam ($params));
      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }
    }
  }

  /**
   * Calls the Video ingest Statistics API.
   * 
   * @param params The parameters to apply to the API request.
   * @param format 'json', 'xml', 'csv', or null.  If null, returns the
   * result of parsing the json results into PHP objects.
   * 
   * See the online documentation for the accepted params and the
   * return data structure.
  */
  public function getIngestStats ($params, $format=null) {
    if ($format == null) {
      return json_decode ($this->getIngestStats ($params, 'json'));
    } else {
      try {
	return $this->getHttp()->get ($this->createScopedUrl("statistics/video_publish.$format"), 
				      $this->addViewAuthParam ($params));
      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }
    }
  }
    
  /**
   * Calls the Video Update API to update the given videoId's video with the given
   * params.
   * 
   * See online documentation for accepted params.
   * 
   * As a convenience, this method makes sure all params
   * are wrappedin video[] before calling the API.  So the following are
   * valid param arrays for updating the title:
   * 
   * array('video[title]' => 'my new title')
   * 
   * array('title' => 'my new title')
   * 
  */
  public function updateVideo ($videoId, $params) {
      try {
	
	$this->getHttp()->put ($this->createUrl ("videos/$videoId"), 
			       $this->addUpdateAuthParam ($this->wrapUpdateVideoParams($params)));
	
      } catch (HttpClientException $e) {
	throw $this->videoApiException ($e);
      }
  }   

  /**
   * Calls the Video Update API, setting the given video's hidden state.
   * 
   * @param visible true if the video should be made viewable (not hidden),
   * false if the video should be made hidden (not viewable).
   * 
  */
  public function setVideoVisible ($videoId, $visible) {
    $this->updateVideo ($videoId,  
			array("video[hidden]" => ($visible ? "false" : "true")));
  }

  /**
   * Calls the Delete Video API.
   * 
   * Moves the video with the given videoId into the trash.
  */
  public function deleteVideo ($videoId) {
    try {
      
      $this->getHttp()->delete ($this->createUrl ("videos/$videoId"), 
				$this->addUpdateAuthParam());
      
    } catch (HttpClientException $e) {
      throw $this->videoApiException ($e);
    }
  }  

  /**
   * Restores the given video from the trash.
   * Videos remain in the trash for 7 days, after which they are permanently
   * deleted.  Only call this method for videos still in the trash.
   */
  public function undeleteVideo ($videoId) {
    $this->updateVideo ($videoId, array('deleted_at' => ''));
  }

  /**
   * Calls the Video Import API.
   * 
   * Imports the videos specified in the given XML document (a String), using
   * the given contributor.
   * 
   * See the online documentation for details of the XML document structure.
  */
  public function importVideosFromXmlString ($xml, $contributor, $params=null) {
    try {
      
      return $this->getHttp()->post ($this->createUrl ("videos/create_many"),
				     $this->addIngestAuthParam ($contributor, $params),
				     "text/xml",
				     $xml);

    } catch (HttpClientException $e) {
      throw $this->videoApiException ($e);
    }
  }

  /**
   * Calls the Video Import API.
   * 
   * Imports the videos specified in the given XML document (a String), using
   * the given contributor.
   * 
   * See the online documentation for details of the XML document structure.
  */
  public function importVideosFromXmlFile ($path, $contributor, $params=null) {
    return $this->importVideosFromXmlString (file_get_contents($path), $contributor, $params);
  }    
  
  /**
   * Calls the Video Import API with an XML document created from the
   * given array of Hash objects, each representing
   * one of the <entry> elements in the import.
   * nested elements like <customdata> should represented by
   * a nested Hash.
   * 
   * for example:
   * 
   * importVideosFromEntries(array(
   * array('src' => "http://www.mywebsite.com/videos/1",
   * 'title' => "video 1",
   * 'tags' => array(array('tag' => 'balloons'))),
   * array('src' => "http://www.mywebsite.com/videos/2",
   * 'title' => "video 2",
   * 'customdata' => array('my_custom_field' => true))
   * );
  */
  public function importVideosFromEntries ($entries, $contributor=null, $params=null) {
    return $this->importVideosFromXmlString ($this->createVideoImportXmlFromHashes($entries), 
					     $contributor,
					     $params);
  }

  /** 
   * Calls the Progressive Download API, downloading one of the given video's assets into a file on the local hard drive.
   * @param video_id the ID of the video to download.
   * @param file_path the path of the local file to create (to download the video asset as)
   * @param selector optional array specifying which asset to download.  Usage here is identical to that of get_download_url.  If omitted, downloads the video's main asset.
   * @param value the parameter whose meaning is defined by selector.
  */
  public function downloadVideoAsset ($videoId, $filepath, $selector=null, $value=null) {
    try {
      return $this->getHttp()->downloadFile ($this->getDownloadUrl($videoId, $selector, $value),
					     $filepath);
    } catch (HttpClientException $e) {
      throw $this->videoApiException ($e);
    }
  }

  /** 
   * Calls the Progressive Download API, downloading the given video's source asset into a file on the local hard drive.
   * @param video_id the ID of the video to download.
   * @param file_path the path of the local file to create (to download the video asset as)
  */
  public function downloadVideoSourceAsset ($videoId, $filepath) {
    return $this->downloadVideoAsset ($videoId, $filepath, 'ext', 'source');
  }

  /**
   Synonym for uploadMedia().
   */
  public function uploadVideo ($path, $contributor, $params=null) {
    return $this->uploadMedia ($path, $contributor, $params);
  }

  /**   
   * Calls the Upload API.
   * 
   * Uploads the media file at the given filepath,
   * using the given contributor as the contributor and the given params
   * to specify custom data for the upload.
   * 
   * To specify an ingest profile, include an 'ingest_profile'
   * in params.  It will be used in the call to authenticate_for_ingest.
   *
   * See the online documentation for the Video Upload API for details.
  */
  public function uploadMedia ($path, $contributor, $params=null) {

    $params = ($params ? $params : array());

    $auth_params = $this->selectAuthParamsFromUploadMediaParams ($params);
    $upload_params = $this->selectNonAuthParamsFromUploadMediaParams ($params);

    $authSig = $this->authenticateForIngest ($contributor, $auth_params);

    try {

      $uploadUrl = $this->getHttp()->get ($this->createUrl ("upload_sessions/$authSig/http_open"), 
					  $upload_params);
      
      $this->getHttp()->postMultipartFileUpload ($uploadUrl, $path);
      
      return trim($this->getHttp()->get ($this->createUrl ("upload_sessions/$authSig/http_close")));
      
    } catch (HttpClientException $e) {
      throw $this->videoApiException ($e);
    }
  }  

  protected function selectAuthParamsFromUploadMediaParams ($params) {
    $result = array();
    foreach ($params as $k=>$v) {
      if ($this->isIngestAuthParamKey($k)) {
	$result[$k] = $v;
      }
    }
    return $result;
  }

  protected function selectNonAuthParamsFromUploadMediaParams ($params) {
    $result = array();
    foreach ($params as $k=>$v) {
      if (! $this->isIngestAuthParamKey($k)) {
	$result[$k] = $v;
      }
    }
    return $result;
  }

  protected function isIngestAuthParamKey($key) {
    return in_array($key, array("ingest_profile"));
  }

  /**
   * Calls the slice API, which cuts a video into pieces, creating a new video ID
   * for each slice.
   * 
   * @param videoId the video to slice
   * @contributor the contribute to use for the newly created segment videos.
   * @param xml the xml to post, conforming to the Video Slice API.
   * @param format (optional) format name specifying the asset to slice.  If omitted, slices the source asset.
   *
   * See the online documentation for details.
  */
  public function sliceVideo ($videoId, $contributor, $xml, $format='source') {    
    try {

      $url = "videos/$videoId/formats/$format/slice.xml";
      
      return $this->getHttp()->post ($this->createUrl ($url),
				     $this->addIngestAuthParam ($contributor),
				     "text/xml",
				     $xml);
      
    } catch (HttpClientException $e) {
      throw $this->videoApiException ($e);
    }
  }

  /**
   * Returns true if the calling browser is an iphone or iPod.
   */
  public function is_user_agent_iphone() {
    $browser = strtolower($_SERVER["HTTP_USER_AGENT"]); 
    return (strpos($browser, 'iphone') !== false ||
	    strpos($browser, 'ipod') !== false);
  }

  ///////////////////////////////////////
  // implementation

  /**
   * @ignore
   */
  protected function __construct ($baseUrl, $accountId, $libraryId, $licenseKey) {
    if (! $baseUrl) {
      throw new VideoApiException ("VideoApi: baseUrl required.");
    }

    if (! $accountId) {
      throw new VideoApiException ("VideoApi: accountId required.");
    }

    if (! $licenseKey) {
      throw new VideoApiException ("VideoApi: licenseKey required.");
    }

    $this->setBaseUrl ($baseUrl);
    $this->accountId = $accountId;
    $this->libraryId = $libraryId;
    $this->licenseKey = $licenseKey;    

    $this->rssAuthRequired = false;
    $this->http = $this->createHttpClient();

    $this->setAuthTokenDurationInMinutes(15);

    $this->viewToken = null;
    $this->updateToken = null;
    $this->ingestToken = null;    
  }  

  /**
   * @ignore
   * Don't call this - public for testing only.  Call authenticateForView().
   */
  public function getViewAuthToken() {
    if ($this->viewToken == null) {
      $this->viewToken = new AuthToken("view_key", $this->getLicenseKey());
    }
    return $this->viewToken;
  }

  /**
   * @ignore
   */
  protected function getUpdateAuthToken() {
    if ($this->updateToken == null) {
      $this->updateToken = new AuthToken("update_key", $this->getLicenseKey());
    }
    return $this->updateToken;
  }

  /**
   * @ignore
   */
  protected function getIngestAuthToken() {
    if ($this->ingestAuthToken == null) {
      $this->ingestAuthToken = new AuthToken("ingest_key", $this->getLicenseKey());
    }
    return $this->ingestAuthToken;
  }

  /**
   * Returns a valid auth signature.  Provide an AuthToken object, the duration in minutes, and params to include other than the duration parameter.
   * This will return the AuthToken's signature if it hasn't expired yet, otherwise will
   * call the API for a new signature and update the token object with it.
   * Throws VideoApiAuthenticationFailedException if call fails or 
   * token isn't valid.
   *
   * @ignore
   */
  protected function getAuthSignature ($token, $duration, $params=null) {    
    $params = ($params ? $params : array());

    // if we have one that isn't expired yet, use it
    if (! $token->isExpired ()) {
      return $token->getToken();
    } else {
      return $token->renew ($this->fetchAuthToken($token->getName(),
						  $duration,
						  $params),
			    $duration);
    }
  }

  protected function fetchAuthToken ($name, $duration, $params) {
    try {
      return $this->getHttp()->get ($this->createUrl("api/$name"), 
				    array_merge ($params, 
						 array('duration' => $duration, 
						       'licenseKey' => $this->getLicenseKey())));

    } catch (HttpClientException $e) {
      try {
	throw $this->videoApiException ($e);
      } catch (VideoApiException $e1) {
	throw new VideoApiAuthenticationFailedException ($e1);
      }
    }
  }

  /**
   * @ignore
   */
  protected function createVideoSearchUrl ($criteria, $format) {
    return $this->createSearchUrl ("videos", $criteria, $format);
  }

  /**
   * @ignore
   */
  protected function createTrackSearchUrl ($criteria, $format) {
    return $this->createSearchUrl ("tracks", $criteria, $format);
  }

  /**
   * @ignore
   */
  protected function createSearchUrl ($type, $criteria, $format) {
    return $this->createScopedUrl ("$type.$format") .
      $this->getHttp()->createQueryString ($this->isAuthRequiredForSearch($format) 
					   
					   ? $this->addViewAuthParam($criteria) 
					   : $criteria);
  }

  /**
   * @ignore
   */
  protected function isAuthRequiredForSearch ($format) {
    // the only case that doesn't require it is rss when rss auth is not required
    return ! ($format == 'rss' && ! $this->isRssAuthRequired());
  }

  /**
   * @ignore
   */
  protected function cleanupItemCustomFields ($item) {
    if (isset($item->custom_fields)) {
      $item->custom_fields = $this->custom_fields_to_hash($item->custom_fields);
    }    
    return $item;
  }

  /**
   * @ignore
   */
  protected function cleanupSearchResults ($items) {
    foreach ($items as $item) {
      $this->cleanupItemCustomFields ($item);
    }  
    return $item;
  }

  /**
   * @ignore
   */
  protected function custom_fields_to_hash($fields) {
    $hash = array();
    foreach ($fields as $field) {
      $hash[$field->name] = $field->value;
    }
    return $hash;
  }

  /**
   * Collects the video IDs of the given videos and returns them in an array.
   * @ignore
   */
  protected static function collectVideoIdsFromVideos ($videos) {
    $ids = array();
    foreach ($videos as $video) {
      $ids[] = $video->{'video-id'};
    }
    return $ids;
  }

  /**
   * @ignore
   */
  protected function wrapUpdateVideoParams ($params) {
    $newParams = array();
    foreach ($params as $k=>$v) {
      if (preg_match('/^video\[.*\]/', $k)) {
	$newParams[$k] = $v;
      } else {
	$newParams["video[$k]"] = $v;
      }
    }
    return $newParams;
  }
  
  /**
   * @ignore
   */
  protected function confirmIsArray ($array, $videoId, $name) {
  
    if (! $array) {
      throw new Exception ("VideoApi.initVideo: video does not contain $name array.  video ID = $videoId");
    }
    
    if (! is_array($array)) {
      throw new Exception ("VideoApi.initVideo: video->$name is not an array!  video ID = $videoId");
    }
  }

  /**
   * @ignore
   */
  protected function createVideoImportXmlFromHashes ($entries) {

    $entryXmls = array();
    foreach ($entries as $entry) {
      $entryXmls[] = $this->createVideoImportXmlFromValue (array('entry' => $entry));
    }

    return '<?xml version="1.0" encoding="UTF-8"?><add><list>' . join("\n", $entryXmls) . '</list></add>';
  }

  /**
   * @ignore
   */
  protected function createVideoImportXmlFromValue ($value) {
    if (is_array($value)) {

      if (isset($value[0])) {
	$children = array();
	foreach ($value as $item) {
	  $children[] = $this->createVideoImportXmlFromValue($item);
	}
	return (join("\n", $children));

      } else {
	$children = array();
	foreach ($value as $k=>$v) {
	  $children[] = "<$k>" . $this->createVideoImportXmlFromValue($v) . "</$k>";
	}
	
	return join("\n", $children);
      }

    } else {
      return '' . $value;
    }
  }

  protected function videoApiException ($httpClientException) {
    if ($httpClientException->getHttpCode() && 
	$httpClientException->getHttpCode() >= 400 &&
	$httpClientException->getHttpCode() < 500) {
      return new VideoApiException ("Server returned HTTP code " . $httpClientException->getHttpCode());
    } else {
      return $httpClientException;
    }
  }
  
  /**
   * @ignore
   */
  protected function createUrl ($subUrl) {
    return substr($subUrl, 0, 7) == "http://"
      ? $subUrl 
      : $this->getBaseUrl() . $subUrl;
  }

  /**
   * @ignore
   */
  protected function createScopedUrl ($subUrl) {
    return $this->createUrl ("companies/" . $this->getAccountId() .
			     ($this->getLibraryId() 
			      ? "/sites/" . $this->getLibraryId() . "/" 
			      : "/") 
			     . $subUrl);
  }

  /**
   * @ignore
   */
  protected function addParam ($param, $value, $params=null) {
    $result = ($params == null ? array() : array_merge($params));
    $result[$param] = $value;
    return $result;
  }
  
  /**
   * @ignore
   */
  protected function addViewAuthParam ($params=null) {
    return $this->addParam ("signature", $this->authenticateForView(), $params);
  }

  /**
   * @ignore
   */
  protected function addUpdateAuthParam ($params=null) {
    return $this->addParam ("signature", $this->authenticateForUpdate(), $params);
  }

  /**
   * @ignore
   */
  protected function addIngestAuthParam ($contributor, $params=null) {
    return $this->addParam ("signature", $this->authenticateForIngest($contributor), $params);
  }

  /**
   * @ignore
   */
  protected function createHttpClient() {
    return new HttpClientCurl();
  }
  
  public function getHttp() {
    return $this->http;
  }
  
  /**
   * Returns the account ID provided to the constructor.
  */
  public function getAccountId() {  
    return $this->accountId;
  }

  /**
   * Returns the ID of this VideoApi object's library.
   * If you created this api object using for_account, scoping the api
   * object to the entire account, this will return null.
  */
  public function getLibraryId() {
    return $this->libraryId;
  }

  public function getLicenseKey() {
    return $this->licenseKey;
  }
  
  /**
   * Returns the base URL provided to the constructor.
  */
  public function getBaseUrl() {
    return $this->baseUrl;
  }
    
  /**
   * @ignore
   */
  protected function setBaseUrl ($baseUrl) {
    $this->baseUrl = (substr($baseUrl, -1) == '/' 
		      ? $baseUrl 
		      : $baseUrl . '/');
  }
  
  /**
   * Returns the flag provided to the constructor, specifying whether
   * this account has RSS authentication turned on, meaning
   * that this VideoApi will obtain and include an authentication signature
   * when producing an RSS URL.
  */
  public function isRssAuthRequired() {
    return $this->rssAuthRequired;
  }

  /**
   * @param rssAuthRequired specifies whether or not your account requires authentication
   * when accessing the RSS API.  This setting is adjustable in the console.
  */
  public function setRssAuthRequired ($required) {
    $this->rssAuthRequired = $required;
  }
}

class VideoApiException extends Exception {

  /**
   * You can call this with either one argument or two.
   * If with one, call it with an excetion or a message.
   * If with two, call it with a message and an Exception.
   */
  public function __construct ($exceptionOrMessage, $exception=null) {
    parent::__construct ($exceptionOrMessage . ": $exception");
  }


}

/**
   * Thrown by VideoApi when it can't obtain an authentication signature.
 */
class VideoApiAuthenticationFailedException extends VideoApiException {

  /**
   * You can call this with either one argument or two.
   * If with one, call it with an excetion or a message.
   * If with two, call it with a message and an Exception.
   */
  public function __construct ($exceptionOrSignature) {
    parent::__construct ("VideoApi authentication API call failed to return valid signature " . 
			 "(possible invalid license key or exceeded maximum number of allowed " . 
			 "authentication signatures per minute)" . 
			 ($exceptionOrSignature == null ? "" : ", api call returned " . $exceptionOrSignature));
  }
}

class AuthToken {

  private $tokenFileDir;
  private $name;
  private $licenseKey;
  private $token;
  private $durationInMinutes;
  private $startTime;

  private function out ($s) {
    global $authTrace;
    if ($authTrace) {
      echo $s . "\n";
    }
  }

  public function __construct ($name, $licenseKey) {    

    $this->tokenFileDir = sys_get_temp_dir();
    $this->name = $name;    
    $this->licenseKey = $licenseKey;

    if ($licenseKey == null || strlen($licenseKey) == 0) {
      throw new VideoApiException ("licenseKey cannot be null");
    }

    if ($name == null || strlen($name) == 0) {
      throw new Exception ("AuthToken: name cannot be null");
    }
  }

  public function getName() {
    return $this->name;
  }

  public function getPath() {    
    return "$this->tokenFileDir/video_api.php.$this->name.$this->licenseKey.json";
  }

  protected function loadCache() {
    $this->out ("loadTokenFromFile");
    $this->reset();

    // cache file doesn't exist yet
    if (! $this->isCacheFilePresent()) {
      return;
    }

    $props = json_decode (file_get_contents ($this->getPath()));

    // if the file is corrupt, delete it to force a lazy-create next time
    if (! $props) {
      unlink($this->getPath());
      return;
    }

    $this->token = $props->token;
    $this->durationInMinutes = $props->duration_in_minutes;
    $this->startTime = $props->start_time;
  }

  protected function writeCache($token, $durationInMinutes, $startTime) {
    $this->out ("writeCache: $token, $durationInMinutes, $startTime");
    $json = json_encode (array ('token' => $token,
				'duration_in_minutes' => $durationInMinutes,
				'start_time' => $startTime));
    $file = fopen($this->getPath(), "w");
    fwrite ($file, $json);
    fclose ($file);
  }

  public function getToken() {
    $this->out ("getToken");
    if (! $this->token) {
      $this->loadCache();
    }
    $this->out ("token: $this->token");
    return $this->token;
  }

  public function getStartTime() {
    $this->out ("getStartTime");
    if (! $this->startTime) {
      $this->loadCache();
    }
    $this->out ("startTime: $this->startTime");
    return $this->startTime;
  }

  public function getDuration() {
    if (! $this->durationInMinutes) {
      $this->loadCache();
    }
    $this->out ("duration: $this->duration");
    return $this->durationInMinutes;
  }

  public function isExpired () {
    $this->out ("isExpired");

    // if we have no token here or on file, we need a new one
    if ($this->getToken() == null) {
      return true;
    }

    $now = time();

    // pad by 30 seconds to avoid the case of the token expiring on the server
    // just after checking it here.  30 seconds allows for plenty of http 
    // connection delay, but also allows for the case of a 1-minte token,
    // which effectively becomes a 30-second token
    $elapsedSeconds = $now - $this->getStartTime() + 30;
    $this->out ("elapsedSeconds=$elapsedSeconds");
    $elapsedMinutes = $elapsedSeconds / 60;
    $this->out ("elapsedMinutes=$elapsedMinutes");
    $expired = ($elapsedMinutes >= $this->getDuration());
    $this->out ("expired: " . ($expired ? 'true' : 'false'));
    return $expired;
  }

  public function renew($token, $durationInMinutes) {
    $this->out ("renew: $token, $durationInMinutes");
    // update the token file
    $this->writeCache (AuthToken::assertValid($token), 
		       $durationInMinutes, 
		       time());

    $this->reset();

    return $this->getToken();
  }

  protected function reset() {
    $this->out ("reset");
    $this->token = null;
    $this->durationInMinutes = null;
    $this->startTime = null;
  }
  
  public function resetCache() {
    $this->writeCache (null, 0, 0);
  }

  public function isCacheFilePresent() {
    return file_exists ($this->getPath());
  }

  /**
   * @ignore
   */
  public static function assertValid ($signature) {

    $trimmed = trim($signature);
    
    if (strlen($trimmed) == 0) {
      throw new VideoApiAuthenticationFailedException ($trimmed);
    }
    
    $containsLetter = false;
    $containsNumber = false;
    $containsOther = false;
    for ($i = 0; $i < strlen($trimmed); $i++) {
      $c = substr($trimmed, $i, 1);
      
      $containsLetter = $containsLetter || AuthToken::isLetter($c);
      $containsNumber = $containsNumber || AuthToken::isDigit($c);
      
      $containsOther = $containsOther || 
	 (! (AuthToken::isLetter($c) || AuthToken::isDigit($c)));	 
    }
    
    if (! ($containsLetter && $containsNumber && (! $containsOther))) {
      throw new VideoApiAuthenticationFailedException ($trimmed);
    }
    
    return $trimmed;
  }

  /**
   * @ignore
   */
  static function isLetter ($char) {
    return strstr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", $char);
  }

  /**
   * @ignore
   */
  static function isDigit($char) {
    return strstr("0123456789", $char);
  }
}

?>