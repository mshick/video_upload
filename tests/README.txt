$Id: README.txt,v 1.1 2008/10/29 22:07:31 jhedstrom Exp $

Testing instructions for the Video Upload Module
------------------------------------------------

In order to test authentication with an external provider (currently
only YouTube is available), the following array should be attached to
the bottom of your settings.php file, and filled in with the
appropriate values for your test account:


/**
 * Video Upload test account.
 */
$GLOBALS['video_upload_test_account'] = array(
  'video_upload_youtube_developer_key' => '',
  'video_upload_youtube_password' => '',
  'video_upload_youtube_username' => '',
);
