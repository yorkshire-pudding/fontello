/**
 * Icon selection 
 */
if(Drupal && jQuery) (function($) {  
  Drupal.behaviors.fontello_select_icon = {
    attach: function(context, settings) {
      $('.fontello-select-icon', context).once('fontello-select-icon', function(){
        var trigger = $(this);
        var select = $('select', trigger).css('opacity', 0);
        if (select.length) {
          var preview = $('<span/>').addClass('dropdown-preview').html('<span class="text"></span><i class="icon icon-admin-select"></i>').appendTo(select.parent()).on('mouseenter.fontello', function(){
            $(this).addClass('hover');
          }).on('mouseleave.fontello', function(){
            $(this).removeClass('hover');
          });
          var previewWidth = 0;
          var dropdown = $('.dropdown', trigger).show();
          var dropdownMenu = $('.dropdown-menu', dropdown);
          var defaultValue = select.val().trim();
          // Process the dropdown items.
          var selected = $();
          var updateClasses = function(li) {
            if (selected.length) {
              selected.removeClass('selected');
            }
            selected = li.addClass('selected');
          }
          var updateScrollTop = function(force) {
            if (selected.length) {
              var force = force || false;
              var menuHeight = dropdownMenu.height();
              var menuScrollTop = dropdownMenu.scrollTop();
              var selectedTop = selected.position().top;
              var selectedHeight = selected.height();
              var selectedPosition = selectedTop + selectedHeight / 2;
              // Scroll up
              if (force || selectedPosition <= menuHeight / 2) {
                if (force || selectedTop < selectedHeight) {
                  dropdownMenu.scrollTop((menuScrollTop + selectedTop) - (menuHeight / 30 + selectedHeight / 2));
                }
              }
              // Scroll down
              else {
                if (selectedTop > menuHeight - selectedHeight) {
                  dropdownMenu.scrollTop(menuScrollTop + selectedHeight);
                }
              }
            }
          }
          var updateSelection = function(element) {
            var label = $();
            var li = $();
            if (element.is('a, label')) {
              label = element;
              li = element.closest('li');
            }
            else {
              li = element;
              label = $('a, label', li);
            }
            if (li.length && label.length) {
              updateClasses(li);
              $('.text', preview).html(label.html());
              select.val(li.attr('data-value')).trigger('change');
            }
          }
          $('li', dropdownMenu).each(function(){
            var li = $(this);
            var liDataValue = li.attr('data-value');
            if (liDataValue !== undefined || liDataValue !== null) {
              var liLabel = $('a, label', li);
              var liLabelWidth = liLabel.outerWidth(true);
              if (liLabelWidth > previewWidth) {
                previewWidth = liLabelWidth;
              }
              if (defaultValue === liDataValue) {
                updateSelection(li);
              }
              // Mouse Enter
              li.on('mouseenter.fontello', function(){
                updateClasses(li);
                return false;
              });
              // Click
              liLabel.on('click.fontello', function(){
                updateSelection($(this));
                dropdown.dropdown('hide');
                return false;
              });
            }
          });
          preview.width(previewWidth);
          dropdownMenu.width(preview.outerWidth()).css({
            'overflow-y': 'auto',
            'overflow-x': 'hidden',
          });
          // Process the dropdown events.
          select.on('focus', function(){
            preview.addClass('focus');
            $(document).off('keydown.dropdown');
            $(document).on('keydown.dropdown', null, function(e) {
              switch(e.which) {
                // Enter or Spacebar
                case 13:
                case 32:
                  trigger.trigger('click');
                  return false;
                  break;
                // Up Arrow
                case 38:
                  var prev = selected.prev();
                  var i = 0;
                  while (prev.hasClass('dropdown-divider')) {
                    i++;
                    prev = prev.prev();
                    if (!prev.length || i > 5) {
                      break;
                    }
                  }
                  if (prev.length) {
                    setTimeout(function(){
                      updateSelection(prev);
                    }, 10);
                  }
                  return false;
                  break;
                // Down Arrow
                case 40:
                  var next = selected.next();
                  var i = 0;
                  while (next.hasClass('dropdown-divider')) {
                    i++;
                    next = next.next();
                    if (!next.length || i > 5) {
                      break;
                    }
                  }
                  if (next.length) {
                    setTimeout(function(){
                      updateSelection(next);
                    }, 10);
                  }
                  return false;
                  break;
              }
            });
          }).on('blur', function(){
            preview.removeClass('focus');
            $(document).off('keydown.dropdown');
          });
          dropdown.hide().on('show', function(event, data){
            preview.addClass('active');
            if (selected.length) {
              updateScrollTop(true);
            }
            // Bind keys for input while open.
            $(document).off('keydown.dropdown');
            $(document).on('keydown.dropdown', null, $(this), function(e) {
              var dropdown = e.data;
              var selected = $('li.selected', dropdown);
              switch(e.which) {
                // Enter or Spacebar
                case 13:
                case 32:
                  updateSelection(selected);
                  dropdown.dropdown('hide');
                  return false;
                  break;
                // Escape
                case 27:
                  dropdown.dropdown('hide');
                  return false;
                  break;
                // Tab
                case 9:
                  dropdown.dropdown('hide');
                  dropdown.closest('.form-item').next().find(':input').focus();
                  return false;
                  break;
                // Up Arrow
                case 38:
                  var prev = selected.prev();
                  var i = 0;
                  while (prev.hasClass('dropdown-divider')) {
                    i++;
                    prev = prev.prev();
                    if (!prev.length || i > 5) {
                      break;
                    }
                  }
                  if (prev.length) {
                    setTimeout(function(){
                      updateSelection(prev);
                      updateScrollTop();
                    }, 10);
                  }
                  return false;
                  break;
                // Down Arrow
                case 40:
                  var next = selected.next();
                  var i = 0;
                  while (next.hasClass('dropdown-divider')) {
                    i++;
                    next = next.next();
                    if (!next.length || i > 5) {
                      break;
                    }
                  }
                  if (next.length) {
                    setTimeout(function(){
                      updateSelection(next);
                      updateScrollTop();
                    }, 10);
                  }
                  return false;
                  break;
              }
            });
          }).on('hide', function(event, data){
            // Unbind dropdown events from document.
            $(document).off('keydown.dropdown');
            preview.removeClass('active');
            updateClasses($('li[data-value=' + select.val().trim() + ']', dropdownMenu));
          });
        }
      });
    }
  };
	
})(jQuery);
