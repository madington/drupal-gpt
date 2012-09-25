<?php

/**
 * Attach GPT targeting form to entities.
 *
 * @param array $entity_type
 * @param object $entity
 * @param array $form
 * @param array $form_state
 * @param string $langcode
 */
function gpt_entity_form($entity_type, $entity, &$form, &$form_state, $langcode) {
  // Certain modules do not use $form_state[$entity_type] for storage of their
  // $entity upon submit, but instead something that is not retrievable
  // (ie custom, such as "term" instead of "taxonomy_term"). Instead of making
  // switch statements that would have to be maintained, we just attach the
  // entity here.
  $form_state[$entity_type] = $entity;

  // Load defaults.
  $defaults = array();
  list($identifier) = entity_extract_ids($entity_type, $entity);
  if ($identifier) {
    $defaults = gpt_entity_crud_load($entity->entity_type, $identifier);
  }
  if (!isset($defaults['targeting']['src'])) {
    $defaults['targeting'] = array('src' => array());
  }

  // Loaded for targeting helpers.
  ctools_include('gpt.admin', 'gpt', FALSE);
  _gpt_admin_attach_css();

  // Define form.
  $form['gpt_entity'] = array(
    '#type' => 'fieldset',
    '#title' => t('Google Publisher Tags'),
    '#group' => 'additional_settings',
    'gpt_entity_wrapper' => array(
      '#prefix' => '<div id="gpt-entity-wrapper">',
      '#suffix' => '</div>',
      '#tree' => TRUE,
      'targeted_ad_unit_override' => array(
        '#type' => 'checkbox',
        '#title' => t('Override targeted ad unit'),
      ),
      'targeted_ad_unit' => array(
        '#type' => 'textfield',
        '#title' => t('Targeted ad unit'),
        '#description' => t('<p>The targeted ad unit and, for Premium publishers, the optional hierarchy behind it.</p><p>SB publishers can utilize one (1) level of hierarchy, while Premium publishers may utilize up to five (5) levels of hierarchy. Do not begin or end with a slash.</p><p>e.g. travel or travel/asia/food</p>'),
      ),
      'async_override' => array(
        '#type' => 'checkbox',
        '#title' => t('Override asynchronous'),
      ),
      'async' => array(
        '#type' => 'checkbox',
        '#title' => t('Asynchronous'),
        '#description' => t('Asynchronous causes non-blocking loading of creatives, option to use Single Request Architecture, and the ability to refresh ads. Some expanding creatives may not expand, if they were not developed as Friendly iFrames.'),
        '#default_value' => 1, // Default to 1 when not overridden.
      ),
      'sra_override' => array(
        '#type' => 'checkbox',
        '#title' => t('Override Single Request Architecture (SRA)'),
      ),
      'sra' => array(
        '#type' => 'checkbox',
        '#title' => t('Single Request Architecture (SRA)'),
        '#description' => t('SRA reduces ad calls to one single request, improving performance of page loads, and also allows guarantee of roadblocks.'),
        '#default_value' => 1, // Default to 1 when not overridden.
      ),
      'collapse_override' => array(
        '#type' => 'checkbox',
        '#title' => t('Override collapse empty divs'),
      ),
      'collapse' => array(
        '#type' => 'checkbox',
        '#title' => t('Collapse empty divs'),
        '#description' => t('Collapse empty div elements when there is no creative to display.'),
      ),
      'gpt_targeting_override' => array(
        '#type' => 'radios',
        '#title' => t('Page-level customized targeting'),
        '#description' => t('<p>Provide key-value pairs for page-level customized targeting. Multiple values may be associated with one key.</p><p>Choosing override will override any page-level customized targeting set elsewhere for this page.</p>'),
        '#options' => array(
          0 => t('Append'),
          1 => t('Override'),
        ),
        '#default_value' => isset($defaults['targeting']['prep']) ? 1 : 0,
      ),
    ),
  );
  // Attach form as a vertical tab, if the form supports them.
  foreach (element_children($form) as $key) {
    if (isset($form[$key]['#type']) && $form[$key]['#type'] == 'vertical_tabs') {
      $form['gpt_entity']['#group'] = $key;
      // TODO: Add JS for tab label.
      break;
    }
  }

  // Short variable for helper.
  $gpt =& $form['gpt_entity']['gpt_entity_wrapper'];

  $overrides = array('targeted_ad_unit', 'async', 'sra', 'collapse');
  foreach ($overrides as $key) {
    $overridden = FALSE;
    $gpt[$key . '_override']['#ajax'] = array(
      'callback' => 'gpt_entity_form_ajax_override_callback',
      'wrapper' => 'gpt-entity-wrapper',
    );

    // If values should be loaded from $defaults.
    if (isset($defaults[$key]) &&
        !isset($form_state['values']['gpt_entity_wrapper'])) {
      $gpt[$key]['#default_value'] = $defaults[$key];
      $overridden = TRUE;
    }
    // Else if the form is submitted and the key has been marked as override.
    elseif (isset($form_state['values']['gpt_entity_wrapper']) &&
        $form_state['values']['gpt_entity_wrapper'][$key . '_override']) {
      $overridden = TRUE;
    }

    // If $key is not overridden then disable.
    if (!$overridden) {
      $gpt[$key]['#access'] = FALSE;
    }
    else {
      $gpt[$key . '_override']['#default_value'] = 1;
    }
  }

  _gpt_targeting_prep($form_state, $defaults['targeting']);
  // TODO: Errors on targeting form upon add more are not displaying.
  _gpt_targeting_form_elements($gpt, $form_state, 'gpt_entity_form_ajax_targeting_callback');

  // TODO: Remove gpt_entity_form_alter() if http://drupal.org/node/1284642 is
  // fixed in core and uncomment the line below.
  //$form['#submit'][] = 'gpt_entity_form_submit';
}

/**
 * Ajax callback from using overrides.
 */
function gpt_entity_form_ajax_override_callback($form, $form_state) {
  return $form['gpt_entity']['gpt_entity_wrapper'];
}

/**
 * Ajax callback from using targeting add more button.
 */
function gpt_entity_form_ajax_targeting_callback($form, $form_state) {
  return $form['gpt_entity']['gpt_entity_wrapper']['gpt_targeting_wrapper'];
}

/**
 * Submit handler for GPT entity form.
 */
function gpt_entity_form_submit_handler(&$form, &$form_state) {
  $data = array();

  // Prepare targeting for storage, iterate over elements.
  $data['targeting'] = array('src' => array());
  foreach ($form_state['values']['gpt_entity_wrapper']['gpt_targeting_wrapper'] as $v) {
    // Only attempt to store the key/value pair if both key and value are set.
    if (!empty($v['key']) && !empty($v['value'])) {
      $data['targeting']['src'][$v['key']][] = array('value' => $v['value'], 'eval' => $v['eval']);
    }
  }
  // If targeting override was selected then prepare values.
  if ($form_state['values']['gpt_entity_wrapper']['gpt_targeting_override']) {
    $data['targeting']['prep'] = gpt_prepare_targeting($data['targeting']['src']);
  }
  // Otherwise if targeting['src'] is empty.
  elseif (empty($data['targeting']['src'])) {
    unset($data['targeting']);
  }

  $overrides = array('targeted_ad_unit', 'async', 'sra', 'collapse');
  foreach ($overrides as $key) {
    if ($form_state['values']['gpt_entity_wrapper'][$key . '_override']) {
      $data[$key] = $form_state['values']['gpt_entity_wrapper'][$key];
    }
  }

  // Retrieve the identifier for the entity.
  list($id) = entity_extract_ids($form['#entity_type'], $form_state[$form['#entity_type']]);

  // If no data, then attempt a delete.
  if (empty($data)) {
    gpt_entity_crud_delete($form['#entity_type'], $id);
  }
  // Else save.
  else {
    gpt_entity_crud_save($form['#entity_type'], $id, $data);
  }
}