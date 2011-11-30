/**
 * JS functionality for the Video Upload Browser method.
 */

var uploadTimeout;

/**
 * Attaches the ahah behavior to each ahah form element.
 */
Drupal.behaviors.videoUpload = function(context) {
  var selector = '.video-upload-new-upload';

  $(selector).each(function() {
    if ($(this).size() && !$(this).hasClass('.video-upload-processed')) {
      Drupal.videoUpload.attachDialogBehaviour(this);
      $(this).addClass('video-upload-processed');
    }
  });

  // set a timeout to prevent expired upload tokens
  if ($('.video-upload-browser-send').length>0) {
    uploadTimeout = setTimeout('Drupal.videoUpload.uploadTimeout()', 300000);
    $('.video-upload-browser-send').click(function() {
      clearTimeout(uploadTimeout);
    });
  }
};

Drupal.videoUpload = function() {};

/**
 * A stack containing the field ids of widgets from which we launched dialogs.
 * This is needed because when an upload finishes, we don't actually know
 * which dialog finished (and we don't care), we just kill one, and fill one
 * empty space in the node add/edit form.
 *
 * This is done this way because we can't pass any params to the YouTube $nextUrl
 * and have them provided there (as a POST or GET param), which sucks.
 */
Drupal.videoUpload.fields = new Array();

/**
 * Initialize JQuery Form and attach it to the form in the dialog.
 */
Drupal.videoUpload.attachDialogBehaviour = function(context) {
  var options = {
    context: context,
    dataType: 'json',
    beforeSubmit:  Drupal.videoUpload.beforeSubmit,
    success:       Drupal.videoUpload.completeUpload
  };

  $('form', context).ajaxForm(options);
}


// JQuery Form presubmit handler
Drupal.videoUpload.beforeSubmit = function(formData, jqForm, options) {
  // Remove previous error message, if any
  $('.upload-validation-message', options.context).remove();

  var fileValue = $('input[type=file]', options.context).attr('value');

  if (fileValue == "") {
    var errorMessage = Drupal.t('No file selected.');
    $('#edit-upload-wrapper', options.context).append('<div class="upload-validation-message">' + errorMessage + '</div>');

    return false;
  }
  
  /**
  * Add client side validation for the input[type=file] accept attribute.
  */
  var accept = $('input[type=file]', options.context).attr('accept').replace(/,\s*/g, '|');

  if (accept.length > 1) {
    var v = new RegExp('\\.(' + accept + ')$', 'gi');
    
    if (!v.test(fileValue)) {
      var errorMessage = Drupal.t("The selected file cannot be uploaded. Extension not allowed.");
      
      $('#edit-upload-wrapper', options.context).append('<div class="upload-validation-message">' + errorMessage + '</div>');
      
      return false;
    }
  }

  Drupal.videoUpload.uploadProgress();

  return true;
}

/**
 * JQuery Form postsubmit handler.
 * Parses the response, closes the dialog, updates the widget on the
 * node add/edit page.
 */
Drupal.videoUpload.completeUpload = function(response, textStatus, XMLHttpRequest) {
  var videoId = response.data.video_id;
  if(videoId) {
    // update out video id array for cool extras
    Drupal.settings.video_upload.field_video_twistage.video_ids.push(videoId);
  }

  var elementId = response.data.element_id;
  $('#' + elementId + '-ahah-wrapper').replaceWith(response.data.field);

}

Drupal.videoUpload.uploadTimeout = function() {
  $('#edit-upload-wrapper').replaceWith('<h2>Upload Timeout!</h2><p>You must refresh this page to upload your video.</p>');
}


// @todo some cleanup needed here
Drupal.videoUpload.uploadProgress = function() {
  var base = 'video-upload-browser-form';
  if (!$('#'+ base + '.ahah-processed').size()) {
    var element_settings = Drupal.settings.video_upload.ahah[base];
    $(element_settings.selector).each(function() {
      element_settings.element = this;
    });

    $('#'+ base).addClass('ahah-processed');
  }

  this.element = element_settings.element;
  this.progress = element_settings.progress;
  Drupal.videoUpload.progressBar(this.element, this.progress);
}

Drupal.videoUpload.progressBar = function(element, progress) {
  var progressBar = new Drupal.progressBar('ahah-progress-' + this.element.id, eval(this.progress.update_callback), this.progress.method, eval(this.progress.error_callback));
  if (this.progress.message) {
    progressBar.setProgress(-1, this.progress.message);
  }
  if (this.progress.url) {
    progressBar.startMonitoring(this.progress.url, this.progress.interval || 1500);
  }
  this.progress.element = $(progressBar.element).addClass('ahah-progress ahah-progress-bar');
  this.progress.object = progressBar;
  
  $(this.element).children('.notice').hide();
  $(this.element).after(this.progress.element);
}