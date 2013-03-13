<?php

/**
 * @file fontello.admin.inc
 * Provides administrative callbacks and tasks.
 */

/**
 * Fontello Admin Overview Form
 */
function fontello_admin_form($form, $form_state, $theme = 'default') {
  // Ensure the build info includes the default value.
  $form_state['build_info']['args'][0] = $theme;
  // Ensure the form is cached so it can validate and submit properly.
  $form['#cache'] = TRUE;
  // Add administrative css.
  fontello_admin_add_css();
  // Add the theme's bundle, if applicable.
  fontello_bundle_css_add($theme, $form, TRUE);
  $bundle = $form_state['bundle'] = fontello_bundle_load($theme);
  // Icons.
  $form['icons'] = array(
    '#access' => fontello_admin_form_default_access($theme, $bundle),
    '#type' => 'fieldset',
    '#title' => t('Icons') . ' (' . ($bundle ? count($bundle['icons']) : '0') . ')',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  if (!$bundle || empty($bundle['icons'])) {
    $form['icons']['#description'] = t('There are currently no font icons available. Please upload the webfont bundle archive provided by !fontello below.', array(
      '!fontello' => l('Fontello', 'http://fontello.com'),
    ));
  }
  if ($bundle && isset($bundle['icons'])) {
    foreach ($bundle['icons'] as $icon) {
      $form['icons'][$icon['css']] = array(
        '#theme' => 'fontello_icon',
        '#icon' => $icon['css'],
        '#bundle' => $bundle,
        '#prefix' => '<div class="iconwrapper" title="icon-' . $icon['css'] . '">',
        '#suffix' => '<span class="iconlabel">icon-<em>' . $icon['css'] . '</em></span></div>',
      );
    }  
  }
  // Upload form.
  $form['bundle'] = array(
    '#type' => 'fieldset',
    '#title' => t('Webfont Bundle'),
    '#collapsible' => TRUE,
    '#collapsed' => fontello_admin_form_default_access($theme, $bundle) && count($bundle['icons']),
  );
  if (!fontello_admin_form_default_access($theme, $bundle)) {
    $form['bundle']['#description'] = t('This theme does not have a specific webfont bundle uploaded. You can upload a new webfont bundle below or this theme will use the !default if it exists.', array(
      '!default' => l('default bundle', 'admin/appearance/fontello'),
    ));
  }
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
  if ($bundle) {
    $header = array(
      t('Files'),
      t('Size'),
    );
    $rows = array();
    $files = file_scan_directory($bundle['uri'], '/.*/', array('nomask' => '/(\.\.?|\.html|\.txt)$/'));
    ksort($files);
    foreach($files as $file) {
      $row = array();
      $filename = str_replace($bundle['uri'] . '/', '', $file->uri);
      if ($filename === 'config.json') {
        $filename .= ' (' . l('Download', 'admin/appearance/fontello/config/' . $bundle['theme']) . ')';
      }
      $row[] = $filename;
      $row[] = format_size(filesize($file->uri));
      $rows[] = $row;
    }
    $form['bundle']['files'] = array(
      '#access' => fontello_admin_form_default_access($theme, $bundle) && count($bundle['icons']),
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
    );
    $form['bundle']['delete'] = array(
      '#access' => fontello_admin_form_default_access($theme, $bundle) && count($bundle['icons']),
      '#type' => 'submit',
      '#value' => t('Delete Bundle'),
      '#submit' => array('fontello_admin_form_delete_submit'),
    );
    // Settings.
    $form['settings'] = array(
      '#access' => fontello_admin_form_default_access($theme, $bundle) && count($bundle['icons']),
      '#type' => 'fieldset',
      '#title' => t('Settings'),
      '#tree' => TRUE,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['settings']['animation'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable Animation'),
      '#description' => t('For browsers that support CSS3 animations, add the class %animation to any icon to enable a spinning animation. This is useful for creating custom throbbers.', array('%animation' => '.animate-spin')),
      '#default_value' => $bundle['animation'],
      '#return_value' => 1,
    );
    $form['settings']['ie7'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable IE7 Support'),
      '#default_value' => $bundle['ie7'],
      '#return_value' => 1,
    );
    $form['settings']['method'] = array(
      '#type' => 'radios',
      '#title' => t('Include Method'),
      '#options' => array(
        0 => t('<strong>Default</strong><div><small>This method is strongly recommended and is usually enough. It contains the @font-face and character codes definitions.</small></div>'),
        1 => t('<strong>Embedded</strong><div><small>Same as Default, however it contains the embedded WOFF font to avoid CORS issues in Firefox and IE9+, when fonts are hosted on a separate domain. It is strongly recommend to resolve this issue by adding appropriate "Access-Control-Allow-Origin" server headers.</small></div>'),
        2 => t('<strong>Codes Only</strong><div><small>This method is for advanced use cases and will require the use of a custom @font-face declaration. This can be useful in cases where seperating out the font is necessary (like automated asset build systems), however still wishing to reap the beneifits of Fontello\'s CSS generation. This module does not yet support a separated font asset upload and will need to be added via the theme or a custom module. See Fontello source code for an example.</small></div>'),
      ),
      '#default_value' => $bundle['method'],
    );
    $form['settings']['tag'] = array(
      '#type' => 'select',
      '#title' => t('HTML Markup'),
      '#description' => t('Choose the HTML markup tag that Fontello icons should be created with. Typically, this is an %i tag, however it can be changed to suite the theme requirements.', array('%i' => '<i/>')),
      '#options' => array(
        'i' => '<i/>',
        'span' => '<span/>',
        'div' => '<div/>',
      ),
      '#default_value' => $bundle['tag'],
    );
    $form['settings']['actions'] = array('#type' => 'actions', '#tree' => FALSE);
    $form['settings']['actions']['save'] = array(
      '#type' => 'submit',
      '#value' => t('Save Settings'),
      '#submit' => array('fontello_admin_form_settings_submit'),
    );
  }
  return $form;
}

function fontello_admin_form_default_access($theme, $bundle) {
  // Always show for the default bundle.
  if ($theme === 'default') {
    return TRUE;
  }
  return !$bundle['default'];
}

/**
 * Admin form import validation.
 */
function fontello_admin_form_import_validate($form, &$form_state) {
  // Validate extensions.
  $validators = array('file_validate_extensions' => array(archiver_get_extensions()));
  if (!$file = file_save_upload('file', $validators, NULL, FILE_EXISTS_REPLACE)) {
    form_set_error('file', t('The file uploaded is not a valid archive.'));
    return;
  }
  $status = fontello_bundle_import($file, $form_state);
  if ($status !== TRUE) {
    form_set_error('file', $status);
    return;
  }
}

/**
 * Admin form import submission.
 */
function fontello_admin_form_import_submit($form, &$form_state) {
  $theme = $form_state['build_info']['args'][0];
  $bundle = $form_state['bundle'];
  if (!empty($bundle['icons'])) {
    $themes = list_themes();
    $theme_name = $theme === 'default' ? 'the default bundle' : $themes[$theme]->info['name'];
    drupal_set_message(format_plural(
      count($bundle['icons']),
      '1 webfont icon was successfully imported into %theme.',
      '@count webfont icons were successfully imported into %theme.',
      array('%theme' => $theme_name)
    ));
  }
  $form_state['input'] = array();
}

/**
 * Admin form delete bundle.
 */
function fontello_admin_form_delete_submit($form, &$form_state) {
  $theme = $form_state['build_info']['args'][0];
  fontello_bundle_delete($theme);
}

/**
 * Admin form import submission.
 */
function fontello_admin_form_settings_submit($form, &$form_state) {
  $theme = $form_state['build_info']['args'][0];
  $bundle = $form_state['bundle'];
  // Exclude unnecessary elements.
  form_state_values_clean($form_state);
  foreach ($form_state['values']['settings'] as $key => $value) {
    $bundle[$key] = $value;
  }
  fontello_bundle_save($theme, $bundle);
}
