<?php

require_once('HttpClientException.php');

class HttpClientCurl {

  private $trace = false;

  /**
   * When true, traces all HTTP calls and responses to stdout.
   * @param bool
   */
  public function traceOn($b) {
    $this->trace = $b;
  }
  
  /**
   * @param string $url
   * @param array $params associative array of params that will be url-encoded and appended to the given url as its query string.
   * @return string the contents of the given URL when requested via an HTTP GET.
   * Throws HttpClientException if the request fails or does not
   * return an HTTP response code considered okay.  See isValidHttpCode().
   */
  public function get ($url, $params=null) {
    return $this->getUrlContents ($url, 'GET', $params);
  }

  /**
   * @param string $url
   * @param array $params associative array of params that will be url-encoded and appended to the given url as its query string.
   * @return string the contents of the given URL when requested via an HTTP PUT.
   * Throws HttpClientException if the request fails or does not
   * return an HTTP response code considered okay.  See isValidHttpCode().
   */
  public function put ($url, $params=null) {
    return $this->getUrlContents ($url, 'PUT', $params);
  }

  /**
   * @param string $url
   * @param array $params associative array of params that will be url-encoded and appended to the given url as its query string.
   * @return string the contents of the given URL when requested via an HTTP DELETE.
   * Throws HttpClientException if the request fails or does not
   * return an HTTP response code considered okay.  See isValidHttpCode().
   */
  public function delete ($url, $params=null) {
    return $this->getUrlContents ($url, 'DELETE', $params);
  }

  /**
   * @param string $url
   * @param array $params associative array of params that will be url-encoded and appended to the given url as its query string.
   * @param string $contentType the Content-Type header value
   * @param string $data the data to POST.
   * @return string the response to the POST request.
   * Throws HttpClientException if the request fails or does not
   * return an HTTP response code considered okay.  See isValidHttpCode().
   */
  public function post ($url, $params, $contentType, $data) {
    return $this->executeCurlQuery ($url, 'POST', $params, $contentType, $data);
  }

  /**
   * Executes a multipart form upload of the given file.
   * @param string $url
   * @param array $params associative array of params that will be url-encoded and appended to the given url as its query string.
   * @param string $filepath the path of the file to POST as a 'file' form variable.
   * @return string the response to the POST request.
   * Throws HttpClientException if the request fails or does not
   * return an HTTP response code considered okay.  See isValidHttpCode().
   */
  public function postMultipartFileUpload ($url, $filepath, $params=null) {
    return $this->post ($url, $params, null, 
			array('file' => "@" . $filepath));
  }

  /**
   * Issues anHTTP GET request to the given URL, downloads the contents of the request to the given filepath.
   * @param string $url 
   * @param string $filepath the path of the file to create with the downloaded data.
   * @param array $params associative array of params that will be url-encoded and appended to the given url as its query string.
   * Throws HttpClientException if the request fails or does not
   * return an HTTP response code considered okay.  See isValidHttpCode().
   */
  public function downloadFile ($url, $filepath, $params=null) {
    return $this->executeCurlQuery ($url, 'GET', $params, null, null, $filepath);
  }

  /**
   * @ignore
   */
  protected function getUrlContents ($url, $method, $params) {
    return $this->executeCurlQuery ($url, $method, $params, null, null);
  }

  /**
   * @ignore
   */
  protected function executeCurlQuery ($url, $method, $params, $contentType, $data, $downloadFilepath=null) {
    
    $fullUrl = $url . $this->createQueryString($params);
    $httpMethod = strtoupper($method);

    if ($this->trace) {
      echo "\nCURL [$httpMethod] $fullUrl\n";
    }

    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $fullUrl);
    curl_setopt($ch, CURLOPT_HEADER, 0);  
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
    
    if ($downloadFilepath != null) {
      $downloadFileHandle = fopen($downloadFilepath, "wb");
      curl_setopt ($ch, CURLOPT_FILE, $downloadFileHandle);
    }

    switch ($httpMethod) {
    case 'GET':
      break;
    case 'PUT':
      curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'PUT');      
      curl_setopt($ch, CURLOPT_HTTPHEADER, array("Content-length: 0"));
      break;
    case 'POST':

      curl_setopt($ch, CURLOPT_POST, true);      

      if ($data == null) {
	throw new HttpClientException ("For post request you must specify data!");
      }
      
      curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
      
      if ($contentType != null) {
	curl_setopt($ch, CURLOPT_HTTPHEADER, array("Content-type: $contentType"));
      }
      
      break;

    case 'DELETE':
      curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'DELETE');
      break;
    }

    $response = curl_exec($ch);
    $info = curl_getinfo($ch);
    $code = isset($info['http_code']) ? $info['http_code'] : -1;
    $error = curl_error($ch);
    curl_close($ch);    

    if ($downloadFileHandle) {
      fclose ($downloadFileHandle);
    }

    if ($response === false || $code < 200 || $code >= 400) {
      throw new HttpClientException ("Request for url $fullUrl failed.", $code);
    } 

    if ($this->trace) {
      echo "\nHTTP response code=" . $info['http_code'] . "\nresponse=$response";
    }

    return $response;
  }

  /**
   * @param array $params an associative array to url-encode and assemble into a url query string including the question mark.  If null, returns a blank string.
   */
  public function createQueryString ($params) {
    return ($params == null ? "" : "?" . http_build_query($params));
  }
}


?>