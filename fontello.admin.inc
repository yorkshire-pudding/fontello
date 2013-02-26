<?php

/**
 * @file fontello.admin.inc
 * Provides administrative callbacks and tasks.
 */

/**
 * Fontello Admin Overview Form
 */
function fontello_admin_form($form, $form_state) {
  $form['#cache'] = TRUE;
  fontello_add_css($form);
  drupal_add_css(drupal_get_path('module', 'fontello') . '/fontello.admin.css');
  $icons = fontello_get_icons();
  $form['#prefix'] = '<div id="fontello-admin-form-wrapper">';
  $form['#suffix'] = '</div>';
  // Icons.
  $form['icons'] = array(
    '#type' => 'fieldset',
    '#title' => t('Icons') . ' (' . count($icons) . ')',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  if (empty($icons)) {
    $form['icons']['#description'] = t('There are currently no font icons available. Please upload the webfont bundle archive provided by !fontello below.', array(
      '!fontello' => l('Fontello', 'http://fontello.com'),
    ));
  }
  foreach ($icons as $icon) {
    $form['icons'][$icon['uid']] = array(
      '#theme' => 'fontello_icon',
      '#icon' => $icon,
      '#prefix' => '<div class="iconwrapper" title="icon-' . $icon['css'] . '">',
      '#attributes' => array(
        'class' => array(
          'icon',
        ),
      ),
      '#suffix' => '<span class="iconlabel">icon-<em>' . $icon['css'] . '</em></span></div>',
    );
  }
  // Upload form.
  $form['bundle'] = array(
    '#type' => 'fieldset',
    '#title' => t('Webfont Bundle'),
    '#collapsible' => TRUE,
    '#collapsed' => count($icons) ? TRUE : FALSE,
  );
  if (fontello_config()) {
    $form['bundle']['config'] = array(
      '#type' => 'item',
      '#markup' => t('<p>!fontello includes a configuration file (config.json) inside the webfont bundle that was originally downloaded. This file contains a JSON list of the selected icons so it can be re-imported back into their site. It allows the icon set to be pruned of unused icons or expanded to utilize even more.</p><strong>!config</strong>', array(
        '!fontello' => l('Fontello', 'http://fontello.com'),
        '!config' => l('Download: config.json', 'admin/appearance/fontello/config'),
      )),
    );
  }
  $form['bundle']['file'] = array(
    '#type' => 'file',
    '#title' => t('Upload Webfont Bundle'),
    '#description' => t('Provide the webfont bundle archive supplied from !fontello.', array('!fontello' => l('Fontello', 'http://fontello.com'))),
    '#size' => 40,
  );
  $form['bundle']['actions'] = array('#type' => 'actions');
  $form['bundle']['actions']['import'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
    '#validate' => array('fontello_admin_form_import_validate'),
    '#submit' => array('fontello_admin_form_import_submit'),
  );
  // Settings.
  $form['settings'] = array(
    '#access' => count($icons) ? TRUE : FALSE,
    '#type' => 'fieldset',
    '#title' => t('Settings'),
    '#tree' => TRUE,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['settings']['fontello_animation'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Animation'),
    '#description' => t('For browsers that support CSS3 animations, add the class %animation to any icon to enable a spinning animation. This is useful for creating custom throbbers.', array('%animation' => '.animate-spin')),
    '#default_value' => variable_get('fontello_animation', 1),
    '#return_value' => 1,
  );
  $form['settings']['fontello_ie7'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable IE7 Support'),
    '#default_value' => variable_get('fontello_ie7', 1),
    '#return_value' => 1,
  );
  $form['settings']['fontello_method'] = array(
    '#type' => 'radios',
    '#title' => t('Include Method'),
    '#description' =>
      t('<p><strong>Default</strong> - This method is strongly recommended and is usually enough. It contains the @font-face and character codes definitions.</p>') .
      t('<p><strong>Embedded</strong> - Same as Default, however it contains the embedded WOFF font to avoid CORS issues in Firefox and IE9+, when fonts are hosted on a separate domain. It is strongly recommend to resolve this issue by adding appropriate "Access-Control-Allow-Origin" server headers. NOTE: Does not contain IE7 codes.</p>') .
      t('<p><strong>Codes Only</strong> - This method is for advanced use cases and will require the use of a custom @font-face declaration. This can be useful in cases where seperating out the font is necessary (like automated asset build systems), however still wishing to reap the beneifits of Fontello\'s CSS generation. This module does not yet support a separated font asset upload and will need to be added via the theme or a custom module. See Fontello source code for an example.</p>'),
    '#options' => array(
      0 => t('Default'),
      1 => t('Embedded'),
      2 => t('Codes Only'),
    ),
    '#default_value' => variable_get('fontello_method', 0),
  );
  $form['settings']['fontello_element'] = array(
    '#type' => 'select',
    '#title' => t('Icon Element'),
    '#description' => t('Choose the markup element that font icons should be created with. Typically, this is an %i tag, however it can be changed to suite the theme requirements.', array('%i' => '<i/>')),
    '#options' => array(
      'i' => '<i/>',
      'span' => '<span/>',
      'div' => '<div/>',
    ),
    '#default_value' => variable_get('fontello_element', 'i'),
  );
  $form['settings']['actions'] = array('#type' => 'actions');
  $form['settings']['actions']['import'] = array(
    '#type' => 'submit',
    '#value' => t('Save Settings'),
    '#submit' => array('fontello_admin_form_settings_submit'),
  );
  return $form;
}

/**
 * Admin form import validation.
 */
function fontello_admin_form_import_validate($form, &$form_state) {
  $old_fontello_dir = variable_get('fontello_dir', NULL);
  // Validate extensions.
  $validators = array('file_validate_extensions' => array(archiver_get_extensions()));
  if (!$file = file_save_upload('file', $validators, NULL, FILE_EXISTS_REPLACE)) {
    form_set_error('file', t('The file uploaded is not a valid archive.'));
    return;
  }
  // Validate the filename.
  if (strpos($file->filename, 'fontello-') === FALSE) {
    form_set_error('file', t('The file upload is not a valid archive provided by !fontello.', array('!fontello' => l('Fontello', 'http://fontello.com'))));
    return;
  }
  // Determine the new Fontello directory.
  $fontello_dir = substr($file->filename, 0, strpos($file->filename, '.'));
  // Prepare the directory
  $archive_destination = 'public://fontello';
  file_prepare_directory($archive_destination);
  // Extract the files.
  module_load_include('inc', 'update', 'update.manager');
  try {
    $archive = update_manager_archive_extract($file->uri, $archive_destination);
  }
  catch (Exception $e) {
    form_set_error('file', $e->getMessage());
    return;
  }
  $files = $archive->listContents();
  if (!$files || !in_array($fontello_dir . '/config.json', $files)) {
    form_set_error('file', t('The archive uploaded does not contain the %config file provided by !fontello.', array(
      '%config' => 'config.json',
      '!fontello' => l('Fontello', 'http://fontello.com'),
    )));
    file_unmanaged_delete_recursive('public://fontello/' . $fontello_dir);
    return;
  }
  // Delete the previous fontello directory.
  if ($old_fontello_dir && $old_fontello_dir !== $fontello_dir) {
    file_unmanaged_delete_recursive('public://fontello/' . $old_fontello_dir);
  }
  // Rename the extracted directory into a unique hash.
  $new_fontello_dir = md5($fontello_dir . REQUEST_TIME);
  if (!@rename(drupal_realpath('public://fontello/' . $fontello_dir), drupal_realpath('public://fontello/' . $new_fontello_dir))) {
    drupal_set_message(t('Failed to rename directory "@old_dir" to "@new_dir"', array('@old_dir' => utf8_encode($fontello_dir), '@new_dir' => utf8_encode($new_fontello_dir))));
  }
  // Save the new fontello directory in a variable.
  variable_set('fontello_dir', $new_fontello_dir);
  // Delete the archive file.
  file_delete($file);
}

/**
 * Admin form import submission.
 */
function fontello_admin_form_import_submit($form, &$form_state) {
  $icons = fontello_get_icons(TRUE);
  if (!empty($icons)) {
    drupal_set_message(format_plural(count($icons), '1 font icon was successfully imported.', '@count font icons were successfully imported.'));
  }
  $form_state['input'] = array();
}

/**
 * Admin form import submission.
 */
function fontello_admin_form_settings_submit($form, &$form_state) {
  // Exclude unnecessary elements.
  form_state_values_clean($form_state);
  foreach ($form_state['values']['settings'] as $key => $value) {
    variable_set($key, $value);
  }
  drupal_set_message(t('Fontello settings have been saved.'));
}
