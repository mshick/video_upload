/**
 * Attaches the preview behaviors
 */
Drupal.behaviors.videoUploadAdmin = function(context) {
  $('.video-upload-admin-thumb').click(function() {
    Drupal.videoUploadAdmin.getPreview(this);
    return false;
  });
}

Drupal.videoUploadAdmin = {
  getPreview: function(context) {
    var pData = $(context).attr('rel').split('|');
    var pDiv = '#' + pData[0];
    var pTitle = $(context).attr('title');

    var pProvider = pData[1];
    var pId = pData[2];
    var url = '/video-upload/preview/' + pId + '/' + pProvider;
        
    $.ajax({
      url: url,
      dataType: 'json',
      success: function(response, status) {
        $(pDiv).html(response.data);
      },
      complete: Drupal.videoUploadAdmin.openDialog(pDiv, pTitle)
    });
  },

  openDialog: function(div, pTitle) {
    $(div).dialog({
      height: 480, 
      width: 680, 
      title: pTitle, 
      modal: true, 
      close: function(event, ui) {
        $(this).dialog('destroy');
      }
    });  
  }
}