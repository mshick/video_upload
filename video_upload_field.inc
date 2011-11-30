<?php

// $Id: video_upload_field.inc,v 1.7.4.2 2010/05/05 00:36:03 jhedstrom Exp $

/**
 * @file video_upload_field.inc
 * Provides filefield hooks.
 */

/**
 * Provides views data to CCK.
 */
function video_upload_video_upload_field_settings_views_data($field) {
  $data = content_views_field_views_data($field);
  $db_info = content_database_info($field);
  $table_alias = content_views_tablename($field);

  // Add a relationship for related file.
  $data[$table_alias][$field['field_name'] .'_fid']['relationship'] = array(
    'base' => 'files',
    'field' => $db_info['columns']['fid']['column'],
    'handler' => 'views_handler_relationship',
  );

  // Video status.
  $data[$table_alias][$field['field_name'] . '_status'] = array(
    'title' => t('Video status'),
    'field' => $db_info['columns']['status']['column'],
    'help' => t('Video Upload - Video status'),
    'filter' => array(
      'handler' => 'views_handler_filter_video_upload_status',
      'label' => t('Video status'),
    ),
  );

  // Video status timestamp.
  $data[$table_alias][$field['field_name'] . '_status_ts']['filter'] = array(
    'title' => t('Video status timestamp'),
    'field' => $db_info['columns']['status_ts']['column'],
    'help'  => t('Video Upload - Video status timestamp'),
    'filter' => array(
      'handler' => 'views_handler_filter_date',
      'label' => t('Video status timestamp'),
    ),
  );

  return $data;
}
