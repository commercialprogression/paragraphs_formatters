<?php

/*
 * @file
 * The field formatters.
 */

/**
 * Implement hook_field_formatter_info().
 */
function paragraphs_formatters_field_formatter_info() {
  return array(
    'paragraphs_formatters_advanced' => array(
      'label' => t('Paragraphs (advanced)'),
      'field types' => array('paragraphs'),
      'settings' => array(
        'view_mode' => 'full',
        'bundles' => array(),
        'max_display' => 0,
        'delta' => 0,
      ),
    ),
  );
}

/**
 * Implements hook_field_formatter_settings_form().
 */
function paragraphs_formatters_field_formatter_settings_form($field, $instance, $view_mode, $form, &$form_state) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];
  $elements = array();

  if ($display['type'] == 'paragraphs_formatters_advanced') {
    $entity_type = entity_get_info('paragraphs_item');
    $options = array();
    foreach ($entity_type['view modes'] as $mode => $info) {
      $options[$mode] = $info['label'];
    }

    $bundles = array();
    foreach ($entity_type['bundles'] as $key => $value) {
      $bundles[$key] = $key;
    }

    $elements['view_mode'] = array(
      '#type' => 'select',
      '#title' => t('View mode'),
      '#options' => $options,
      '#default_value' => $settings['view_mode'],
      '#description' => t('Select the view mode'),
    );

    $elements['bundles'] = array(
      '#type' => 'select',
      '#title' => t('Bundle(s) to show'),
      '#multiple' => TRUE,
      '#options' => $bundles,
      '#default_value' => $settings['bundles'],
      '#description' => t('Leave blank to show all.'),
    );

    $elements['max_display'] = array(
      '#title' => t('(Maximum) Number of items to display'),
      '#type' => 'select',
      '#default_value' => $settings['max_display'],
      '#options' => array(
        0 => 'Unlimited',
        1 => 1,
        2 => 2,
        3 => 3,
        4 => 4,
        5 => 5,
      ),
      '#required' => TRUE,
    );

    $elements['delta'] = array(
      '#title' => t('Start at delta'),
      '#type' => 'select',
      '#default_value' => $settings['delta'],
      '#options' => array(
        0 => 0,
        1 => 1,
        2 => 2,
        3 => 3,
        4 => 4,
        5 => 5,
      ),
      '#required' => TRUE,
    );
  }

  return $elements;
}

/**
 * Implements hook_field_formatter_settings_summary().
 */
function paragraphs_formatters_field_formatter_settings_summary($field, $instance, $view_mode) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];

  $module_name = 'paragraphs_formatters';
  $form = call_user_func_array($module_name . '_field_formatter_settings_form', array(
    $field,
    $instance,
    $view_mode,
    array(),
    array(),
  ));

  $summary = array();
  switch ($display['type']) {
    case 'paragraphs_formatters_advanced':
      foreach ($settings as $key => $setting) {
        if (is_array($setting)) {
          $setting = implode(', ', $setting);
        }
        
        $summary[] = t('@key: @setting', array(
          '@key' => $form[$key]['#title'],
          '@setting' => $setting,
        ));
      }

      break;
  }

  return implode('<br />', $summary);
}

/**
 * Implements hook_field_formatter_prepare_view().
 */
function paragraphs_formatters_field_formatter_prepare_view($entity_type, $entities, $field, $instances, $langcode, &$items, $displays) {
  $display = array_shift($displays);
  $settings = $display['settings'];

  foreach ($items as $delta => $graphs) {
    if ($settings['max_display'] == 0) {
      $items[$delta] = array_slice($items[$delta], $settings['delta']);
    }
    else {
      $items[$delta] = array_slice($items[$delta], $settings['delta'], $settings['max_display']);
    }
  }
}

/**
 * Implements hook_field_formatter_view().
 */
function paragraphs_formatters_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();
  $settings = $display['settings'];

  switch ($display['type']) {
    case 'paragraphs_formatters_advanced':

      // Prevent displaying useless markup if we don't have any values.
      if (empty($items)) {
        return $element;
      }

      // Get view mode from entity.
      $display_view_mode = empty($display['settings']['view_mode']) ? 'full' : $display['settings']['view_mode'];
      // Get view mode from field instance (if configured).
      $view_mode = empty($instance['display'][$display_view_mode]['settings']['view_mode']) ? $display_view_mode : $instance['display'][$display_view_mode]['settings']['view_mode'];

      // Attributes to send to preprocess functions.
      $element['#theme_wrappers'] = array('paragraphs_items');
      $element['#attributes']['class'][] = drupal_clean_css_identifier('paragraphs-items');
      $element['#attributes']['class'][] = drupal_clean_css_identifier('paragraphs-items-view-mode-' . $view_mode);
      $element['#attributes']['class'][] = drupal_clean_css_identifier('paragraphs-items-field-' . $instance['field_name']);

      foreach ($items as $delta => $item) {
        if ($paragraph = paragraphs_field_get_entity($item)) {
          if (entity_access('view', 'paragraphs_item', $paragraph)) {
            // Condition to limit by bundle.
            if (!count($display['settings']['bundles']) || in_array($paragraph->bundle, $display['settings']['bundles'])) {
              $element[$delta]['entity'] = $paragraph->view($view_mode);
            }
          }
        }
      }
      break;
  }
  return $element;
}
