<?php
// $Id: views_handler_filter_video_upload_status.inc,v 1.1 2008/10/21 00:31:05 jhedstrom Exp $
/**
 * Filter by video status.
 */
class views_handler_filter_video_upload_status extends views_handler_filter_in_operator {
  function get_value_options() {
    if (!isset($this->value_options)) {
      $this->value_title = t('Video status');
      $statuses = video_upload_get_all_statuses();
      foreach ($statuses as $status => $description) {
        $options[$status] = $description;
      }
      $this->value_options = $options;
    }
  }
}