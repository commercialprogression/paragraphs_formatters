<?php

/**
 * @file
 * Holds functions for the paragraphs widgets.
 */

/**
 * Implements hook_field_widget_info().
 */
function paragraphs_formatters_field_widget_info() {
  return array(
    'paragraphs_formatters_embed' => array(
      'label' => t('Streamlined'),
      'field types' => array('paragraphs'),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_CUSTOM,
        'default value' => FIELD_BEHAVIOR_NONE,
      ),
    ),
  );
}

/**
 * Implements hook_field_widget_form().
 */
function paragraphs_formatters_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  switch ($instance['widget']['type']) {
    case 'paragraphs_formatters_embed':
      $form['#entity'] = $element['#entity'];
      $return = paragraphs_formatters_field_multiple_value_form($field, $instance, $langcode, $items, $form, $form_state);
      
      // Add module CSS to make everything look nice.
      $return['#attached']['css'] = array(
        drupal_get_path('module', 'paragraphs_formatters') . '/paragraphs_formatters.css'
      );

      return $return;
      break;
  }
}

/**
 * Special handling to create form elements for multiple values.
 */
function paragraphs_formatters_field_multiple_value_form($field, $instance, $langcode, $items, &$form, &$form_state) {
  $field_elements = paragraphs_field_multiple_value_form($field, $instance, $langcode, $items, $form, $form_state);
  return $field_elements;
}

/**
 * Widget form implementation for paragraphs.
 */
function paragraphs_formatters_field_widget_form_build(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $element = paragraphs_field_widget_form_build($form, $form_state, $field, $instance, $langcode, $items, $delta, $element);
  
  // Move the UI elements around.
  unset($element['paragraph_bundle_title']);
  $element['actions']['#weight'] = -1;
  
  return $element;
}

/**
 * #pre_render callback that ensures the element is rendered as being required.
 */
function paragraphs_formatters_field_widget_render_required($element) {
  $element['#required'] = TRUE;
  return $element;
}

/**
 * FAPI validation of an individual paragraph element.
 */
function paragraphs_formatters_field_widget_embed_validate($element, &$form_state, $complete_form) {
  $instance = field_widget_instance($element, $form_state);
  $field = field_widget_field($element, $form_state);
  $field_parents = $element['#field_parents'];
  $field_name = $element['#field_name'];
  $language = $element['#language'];

  $field_state = field_form_get_state($field_parents, $field_name, $language, $form_state);
  if (isset($field_state['entity'][$element['#delta']])) {
    $paragraph_item = $field_state['entity'][$element['#delta']];

    // Now validate elements if the entity is not empty.
    if (((!isset($paragraph_item->removed) || !$paragraph_item->removed) && (!isset($paragraph_item->confirmed_removed) || !$paragraph_item->confirmed_removed))) {

      // Attach field API validation of the embedded form.
      field_attach_form_validate('paragraphs_formatters_item', $paragraph_item, $element, $form_state);

      if (!empty($element['#paragraphs_formatters_required_elements'])) {
        foreach ($element['#paragraphs_formatters_required_elements'] as &$elements) {

          // Copied from _form_validate().
          if (isset($elements['#needs_validation'])) {
            $is_empty_multiple = (!count($elements['#value']));
            $is_empty_string = (is_string($elements['#value']) && drupal_strlen(trim($elements['#value'])) == 0);
            $is_empty_value = ($elements['#value'] === 0);
            if ($is_empty_multiple || $is_empty_string || $is_empty_value) {
              if (isset($elements['#title'])) {
                $error_text = t('!name field is required.', array('!name' => $elements['#title']));
                form_error($elements, filter_xss_admin($error_text));
              }
              else {
                form_error($elements);
              }
            }
          }
        }
      }
    }

    // Only if the form is being submitted, finish the collection entity and
    // prepare it for saving.
    if ($form_state['submitted'] && !form_get_errors()) {

      field_attach_submit('paragraphs_formatters_item', $paragraph_item, $element, $form_state);

      // Load initial form values into $item, so any other form values below the
      // same parents are kept.
      $item = drupal_array_get_nested_value($form_state['values'], $element['#parents']);

      // Set the _weight if it is a multiple field.
      if (isset($element['_weight'])) {
        $item['_weight'] = $element['_weight']['#value'];
      }

      // Put the paragraph item in $item['entity'], so it is saved with
      // the host entity via hook_field_presave() / field API if it is not empty.
      // @see paragraph_field_presave()
      $item['entity'] = $paragraph_item;
      form_set_value($element, $item, $form_state);
    }
  }
}

/**
 * Submit function to add another paragraph.
 * @param $form
 * @param $form_state
 */
function paragraphs_formatters_add_more_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];

  // Go one level up in the form, to the widgets container.
  $element = drupal_array_get_nested_value($form, array_slice($button['#array_parents'], 0, -2));
  $field_name = $element['#field_name'];
  $langcode = $element['#language'];
  $parents = $element['#field_parents'];

  // Increment the items count.
  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);

  $field = $field_state['field'];

  if ($field['cardinality'] == FIELD_CARDINALITY_UNLIMITED || $field_state['real_items_count'] < $field['cardinality']) {
    $field_state['items_count']++;
  }

  field_form_set_state($parents, $field_name, $langcode, $form_state, $field_state);

  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax callback in response to a new empty widget being added to the form.
 *
 * This returns the new page content to replace the page content made obsolete
 * by the form submission.
 *
 * @see field_add_more_submit()
 */
function paragraphs_formatters_add_more_js($form, $form_state) {
  $button = $form_state['triggering_element'];

  // Go one level up in the form, to the widgets container.
  $element = drupal_array_get_nested_value($form, array_slice($button['#array_parents'], 0, -2));
  $field_name = $element['#field_name'];
  $langcode = $element['#language'];
  $parents = $element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);

  $field = $field_state['field'];

  // Add a DIV around the delta receiving the Ajax effect.
  $delta = $element['#max_delta'];
  $element[$delta]['#prefix'] = '<div class="ajax-new-content">' . (isset($element[$delta]['#prefix']) ? $element[$delta]['#prefix'] : '');
  $element[$delta]['#suffix'] = (isset($element[$delta]['#suffix']) ? $element[$delta]['#suffix'] : '') . '</div>';

  return $element;
}


/**
 * Submit callback to remove an item from the field UI multiple wrapper.
 *
 * When a remove button is submitted, we need to find the item that it
 * referenced and delete it. Since field UI has the deltas as a straight
 * unbroken array key, we have to renumber everything down. Since we do this
 * we *also* need to move all the deltas around in the $form_state['values']
 * and $form_state['input'] so that user changed values follow. This is a bit
 * of a complicated process.
 */
function paragraphs_formatters_remove_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];
  $delta = $button['#delta'];

  // Where in the form we'll find the parent element.
  $address = array_slice($button['#array_parents'], 0, -3);

  // Go one level up in the form, to the widgets container.
  $parent_element = drupal_array_get_nested_value($form, $address);
  $field_name = $parent_element['#field_name'];
  $langcode = $parent_element['#language'];
  $parents = $parent_element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);

  if (isset($field_state['entity'][$delta])) {
    $field_state['entity'][$delta]->removed = 1;
  }

  // Fix the weights. Field UI lets the weights be in a range of
  // (-1 * item_count) to (item_count). This means that when we remove one,
  // the range shrinks; weights outside of that range then get set to
  // the first item in the select by the browser, floating them to the top.
  // We use a brute force method because we lost weights on both ends
  // and if the user has moved things around, we have to cascade because
  // if I have items weight weights 3 and 4, and I change 4 to 3 but leave
  // the 3, the order of the two 3s now is undefined and may not match what
  // the user had selected.
  $input = drupal_array_get_nested_value($form_state['input'], $address);
  // Sort by weight,
  // but first remove garbage values to ensure proper '_weight' sorting
  unset($input['add_more']);
  uasort($input, '_field_sort_items_helper');

  // Reweight everything in the correct order.
  $weight = -1 * $field_state['items_count'] + 1;
  foreach ($input as $key => $item) {
    if ($item) {
      $input[$key]['_weight'] = $weight++;
    }
  }

  drupal_array_set_nested_value($form_state['input'], $address, $input);
  field_form_set_state($parents, $field_name, $langcode, $form_state, $field_state);

  $form_state['rebuild'] = TRUE;
}

/**
 * Submit callback to editing an item from the field UI multiple wrapper.
 *
 * When a edited button is submitted, we need to find the item that it
 * referenced and delete it. Since field UI has the deltas as a straight
 * unbroken array key, we have to renumber everything down. Since we do this
 * we *also* need to move all the deltas around in the $form_state['values']
 * and $form_state['input'] so that user changed values follow. This is a bit
 * of a complicated process.
 */
function paragraphs_formatters_edit_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];
  $delta = $button['#delta'];

  // Where in the form we'll find the parent element.
  $address = array_slice($button['#array_parents'], 0, -3);

  // Go one level up in the form, to the widgets container.
  $parent_element = drupal_array_get_nested_value($form, $address);
  $field_name = $parent_element['#field_name'];
  $langcode = $parent_element['#language'];
  $parents = $parent_element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);

  if (isset($field_state['entity'][$delta])) {
    $field_state['entity'][$delta]->being_edited = 1;
  }

  // Fix the weights. Field UI lets the weights be in a range of
  // (-1 * item_count) to (item_count). This means that when we remove one,
  // the range shrinks; weights outside of that range then get set to
  // the first item in the select by the browser, floating them to the top.
  // We use a brute force method because we lost weights on both ends
  // and if the user has moved things around, we have to cascade because
  // if I have items weight weights 3 and 4, and I change 4 to 3 but leave
  // the 3, the order of the two 3s now is undefined and may not match what
  // the user had selected.
  $input = drupal_array_get_nested_value($form_state['input'], $address);
  // Sort by weight,
  // but first remove garbage values to ensure proper '_weight' sorting
  unset($input['add_more']);
  uasort($input, '_field_sort_items_helper');

  // Reweight everything in the correct order.
  $weight = -1 * $field_state['items_count'] + 1;
  foreach ($input as $key => $item) {
    if ($item) {
      $input[$key]['_weight'] = $weight++;
    }
  }

  drupal_array_set_nested_value($form_state['input'], $address, $input);
  field_form_set_state($parents, $field_name, $langcode, $form_state, $field_state);

  $form_state['rebuild'] = TRUE;
}

/**
 * Submit callback to collapse an item from the field UI multiple wrapper.
 *
 * When a collapse button is submitted, we need to find the item that it
 * referenced and delete it. Since field UI has the deltas as a straight
 * unbroken array key, we have to renumber everything down. Since we do this
 * we *also* need to move all the deltas around in the $form_state['values']
 * and $form_state['input'] so that user changed values follow. This is a bit
 * of a complicated process.
 */
function paragraphs_formatters_collapse_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];
  $delta = $button['#delta'];

  // Where in the form we'll find the parent element.
  $address = array_slice($button['#array_parents'], 0, -3);

  // Go one level up in the form, to the widgets container.
  $parent_element = drupal_array_get_nested_value($form, $address);
  $field_name = $parent_element['#field_name'];
  $langcode = $parent_element['#language'];
  $parents = $parent_element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);

  if (isset($field_state['entity'][$delta])) {
    $field_state['entity'][$delta]->being_edited = 0;
    $field_state['entity'][$delta]->must_be_saved = 1;
  }

  // Fix the weights. Field UI lets the weights be in a range of
  // (-1 * item_count) to (item_count). This means that when we remove one,
  // the range shrinks; weights outside of that range then get set to
  // the first item in the select by the browser, floating them to the top.
  // We use a brute force method because we lost weights on both ends
  // and if the user has moved things around, we have to cascade because
  // if I have items weight weights 3 and 4, and I change 4 to 3 but leave
  // the 3, the order of the two 3s now is undefined and may not match what
  // the user had selected.
  $input = drupal_array_get_nested_value($form_state['input'], $address);
  // Sort by weight,
  // but first remove garbage values to ensure proper '_weight' sorting
  unset($input['add_more']);
  uasort($input, '_field_sort_items_helper');

  // Reweight everything in the correct order.
  $weight = -1 * $field_state['items_count'] + 1;
  foreach ($input as $key => $item) {
    if ($item) {
      $input[$key]['_weight'] = $weight++;
    }
  }

  drupal_array_set_nested_value($form_state['input'], $address, $input);
  field_form_set_state($parents, $field_name, $langcode, $form_state, $field_state);

  $form_state['rebuild'] = TRUE;
}

/**
 * Submit callback to remove an item from the field UI multiple wrapper.
 *
 * When a remove button is submitted, we need to find the item that it
 * referenced and delete it. Since field UI has the deltas as a straight
 * unbroken array key, we have to renumber everything down. Since we do this
 * we *also* need to move all the deltas around in the $form_state['values']
 * and $form_state['input'] so that user changed values follow. This is a bit
 * of a complicated process.
 */
function paragraphs_formatters_deleteconfirm_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];
  $delta = $button['#delta'];

  // Where in the form we'll find the parent element.
  $address = array_slice($button['#array_parents'], 0, -3);

  // Go one level up in the form, to the widgets container.
  $parent_element = drupal_array_get_nested_value($form, $address);
  $field_name = $parent_element['#field_name'];
  $langcode = $parent_element['#language'];
  $parents = $parent_element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);

  if (isset($field_state['entity'][$delta])) {
    $field_state['entity'][$delta]->removed = 1;
    $field_state['entity'][$delta]->confirmed_removed = 1;
  }

  // Fix the weights. Field UI lets the weights be in a range of
  // (-1 * item_count) to (item_count). This means that when we remove one,
  // the range shrinks; weights outside of that range then get set to
  // the first item in the select by the browser, floating them to the top.
  // We use a brute force method because we lost weights on both ends
  // and if the user has moved things around, we have to cascade because
  // if I have items weight weights 3 and 4, and I change 4 to 3 but leave
  // the 3, the order of the two 3s now is undefined and may not match what
  // the user had selected.
  $input = drupal_array_get_nested_value($form_state['input'], $address);
  // Sort by weight,
  // but first remove garbage values to ensure proper '_weight' sorting
  unset($input['add_more']);
  uasort($input, '_field_sort_items_helper');

  // Reweight everything in the correct order.
  $weight = -1 * $field_state['items_count'] + 1;
  foreach ($input as $key => $item) {
    if ($item) {
      $input[$key]['_weight'] = $weight++;
    }
  }

  drupal_array_set_nested_value($form_state['input'], $address, $input);
  field_form_set_state($parents, $field_name, $langcode, $form_state, $field_state);

  $form_state['rebuild'] = TRUE;
}

/**
 * Submit function to restore a paragraph that was deleted.
 * @param $form
 * @param $form_state
 */
function paragraphs_formatters_restore_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];
  $delta = $button['#delta'];

  // Where in the form we'll find the parent element.
  $address = array_slice($button['#array_parents'], 0, -3);

  // Go one level up in the form, to the widgets container.
  $parent_element = drupal_array_get_nested_value($form, $address);
  $field_name = $parent_element['#field_name'];
  $langcode = $parent_element['#language'];
  $parents = $parent_element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);


  if (isset($field_state['entity'][$delta])) {
    $field_state['entity'][$delta]->removed = 0;
  }


  // Fix the weights. Field UI lets the weights be in a range of
  // (-1 * item_count) to (item_count). This means that when we remove one,
  // the range shrinks; weights outside of that range then get set to
  // the first item in the select by the browser, floating them to the top.
  // We use a brute force method because we lost weights on both ends
  // and if the user has moved things around, we have to cascade because
  // if I have items weight weights 3 and 4, and I change 4 to 3 but leave
  // the 3, the order of the two 3s now is undefined and may not match what
  // the user had selected.
  $input = drupal_array_get_nested_value($form_state['input'], $address);
  // Sort by weight,
  // but first remove garbage values to ensure proper '_weight' sorting
  unset($input['add_more']);
  uasort($input, '_field_sort_items_helper');

  // Reweight everything in the correct order.
  $weight = -1 * $field_state['items_count'] + 1;
  foreach ($input as $key => $item) {
    if ($item) {
      $input[$key]['_weight'] = $weight++;
    }
  }

  drupal_array_set_nested_value($form_state['input'], $address, $input);
  field_form_set_state($parents, $field_name, $langcode, $form_state, $field_state);

  $form_state['rebuild'] = TRUE;
}
