// $Id: video_upload.js,v 1.4 2008/06/13 18:15:14 jhedstrom Exp $

/**
 * @file
 * jquery for Video Upload module
 */

/**
 * Attach validation behavior to the form
 */
Drupal.behaviors.videoUploadValidateAccept = function() {
  /**
   * Client-side validation of the input (if type = file) accept
   * attribute
   */
  $("input[@type='file'].video-upload").change(function() {
    Drupal.videoUploadValidateFile(this)
  });
}

/**
 * Attempt to validate file type based on the attach attribute of the
 * file tag
 */
Drupal.videoUploadValidateFile = function(file) {
  $('.video-upload-js-error').remove();
  if (file.accept.length > 1){
    acc = new RegExp('\\.(' + (file.accept ? file.accept : '') + ')$','gi');
    if (!acc.test(file.value)) {
      alert('The file ' + file.value + ' does not appear to be a video file.\n'
        + 'Please use a video file with one of these extensions: \n' + file.accept.replace(/\|/g, ', ')
      );
      file.value = ''; 
      return false;
    }
  }
  return true;   
};