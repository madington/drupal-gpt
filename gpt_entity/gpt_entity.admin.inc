<?php

/**
 * Attach our settings form to the ad manager settings form.
 */
function gpt_entity_global_settings_form(&$form, &$form_state) {
  $form['gpt_entity'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#title' => t('GPT Entity Targeting'),
  );
  $form['gpt_entity']['gpt_entity_settings'] = array(
    '#type' => 'subform',
    '#subform_id' => 'gpt_entity_settings_form',
    '#subform_file' => array('module' => 'gpt_entity', 'name' => 'gpt_entity.admin.inc'),
    '#subform_default_triggering_element' => array('submit'),
    '#required' => TRUE,
  );
  // Enable error validation.
  $form['submit']['#submit'][] = 'subform_submit_all';
}

/**
 * Defines settings for this module.
 */
function gpt_entity_settings_form($form, &$form_state, $settings) {
  $form['#tree'] = TRUE;
  $defaults = gpt_entity_get_attach_form_settings();

  // Define the container which will allow us to collect the settings as a
  // single array.
  $form['gpt_entity__attach_form_settings'] = array(
    '#title' => 'Entities',
    '#type' => 'fieldset',
  );

  // Provide sub-fieldests for each entity type.
  foreach (entity_get_info() as $type => $info) {
    // Define our options.
    $options = array('__all__' => 'All bundles');
    foreach ($info['bundles'] as $name => $bundle) {
      $options[$name] = $bundle['label'];
    }

    // Create the fieldset.
    $form['gpt_entity__attach_form_settings'][$type] = array(
      '#collapsed' => TRUE,
      '#collapsible' => TRUE,
      '#title' => $info['label'],
      '#type' => fieldset,
    );

    // Create the form element.
    $form['gpt_entity__attach_form_settings'][$type]['settings'] = array(
      '#default_value' => (
        isset($defaults[$type]) ? $defaults[$type]['settings'] : array()
      ),
      '#description' => t('Select the bundles for which you would like to specify targeting information.'),
      '#options' => $options,
      '#title' => $info['label'],
      '#type' => 'checkboxes',
    );
  }

  // Add system settings form but remove added buttons.
  $actions = (array_key_exists('actions', $form)) ? $form['actions'] : NULL;
  $form = system_settings_form($form);
  $form['actions'] = $actions;

  return $form;
}
