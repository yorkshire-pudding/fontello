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
  fontello_admin_add_css();
  fontello_add_css($form);
  $icons = fontello_get_icons();
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
  $form['bundle']['file'] = array(
    '#type' => 'file',
    '#title' => t('Import Bundle'),
    '#description' => t('Provide the webfont bundle archive supplied from !fontello.', array('!fontello' => l('Fontello', 'http://fontello.com'))),
    '#size' => 40,
  );
  $form['bundle']['import'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
    '#validate' => array('fontello_admin_form_import_validate'),
    '#submit' => array('fontello_admin_form_import_submit'),
  );
  if ($fontello_dir = variable_get('fontello_dir', NULL)) {
    $header = array(
      t('Files'),
      t('Size'),
    );
    $rows = array();
    $files = file_scan_directory('public://fontello/' . $fontello_dir, '/.*/', array('nomask' => '/(\.\.?|\.html|\.txt)$/'));
    ksort($files);
    foreach($files as $file) {
      $row = array();
      $filename = str_replace('public://fontello/' . $fontello_dir . '/', '', $file->uri);
      if ($filename === 'config.json') {
        $filename .= ' (' . l('Download', 'admin/appearance/fontello/config') . ')';
      }
      $row[] = $filename;
      $row[] = format_size(filesize($file->uri));
      $rows[] = $row;
    }
    $form['bundle']['files'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
    );
    $form['bundle']['delete'] = array(
      '#access' => count($icons) ? TRUE : FALSE,
      '#type' => 'submit',
      '#value' => t('Delete Bundle'),
      '#submit' => array('fontello_admin_form_delete_submit'),
    );
  }
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
    '#options' => array(
      0 => t('<strong>Default</strong><div><small>This method is strongly recommended and is usually enough. It contains the @font-face and character codes definitions.</small></div>'),
      1 => t('<strong>Embedded</strong><div><small>Same as Default, however it contains the embedded WOFF font to avoid CORS issues in Firefox and IE9+, when fonts are hosted on a separate domain. It is strongly recommend to resolve this issue by adding appropriate "Access-Control-Allow-Origin" server headers. NOTE: Does not contain IE7 codes.</small></div>'),
      2 => t('<strong>Codes Only</strong><div><small>This method is for advanced use cases and will require the use of a custom @font-face declaration. This can be useful in cases where seperating out the font is necessary (like automated asset build systems), however still wishing to reap the beneifits of Fontello\'s CSS generation. This module does not yet support a separated font asset upload and will need to be added via the theme or a custom module. See Fontello source code for an example.</small></div>'),
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
  // Validate extensions.
  $validators = array('file_validate_extensions' => array(archiver_get_extensions()));
  if (!$file = file_save_upload('file', $validators, NULL, FILE_EXISTS_REPLACE)) {
    form_set_error('file', t('The file uploaded is not a valid archive.'));
    fontello_remove_bundle();
    return;
  }
  $bundle = fontello_import_bundle($file);
  if ($bundle !== TRUE) {
    form_set_error('file', $bundle);
    fontello_remove_bundle();
    return;
  }
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
 * Admin form delete bundle.
 */
function fontello_admin_form_delete_submit($form, &$form_state) {
  fontello_remove_bundle();
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
