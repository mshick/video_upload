<?php

class HttpClientException extends Exception {

  public function __construct ($message, $code=null) {
    parent::__construct ("$message, code=$code");
    $this->httpCode = $code;
  }

  public function getHttpCode() {
    return $this->httpCode;
  }
}

?>