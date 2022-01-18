var sections = [
  '#first'
];
var initialDelay = 5000;
var switchDelay = 5000;
var count = 0;

// Used to switch tabs periodically
function autoSwitchTab(sectionIndex) {
  var section = $(sections[sectionIndex]);
  var index = parseInt(section.find('.placeholder .panel').attr('class').split(' ')[1]);

  // Switch to the next panel
  index = (index + 1) % 1;
  count += 1;

  console.log("Count: " + count);

  let visible = section.find('.placeholder .panel')[0];
  let replacement = section.find('.panels .panel').clone()[index];
  $(replacement).find('.caption').text('Count ' + count);

  $(replacement).fadeOut('slow');
  
  $(visible).fadeOut('slow', function() {
    visible.replaceWith(replacement);
    var replaced = section.find('.placeholder .panel')[0];
    $(replaced).fadeIn('slow');
  });
  
  setTimeout(function() {
    autoSwitchTab(sectionIndex);
  }, switchDelay);
}

// Initialisation
$(document).ready(function() {
  console.log("Applying click events");

  for (let sectionIndex = 0; sectionIndex < sections.length; sectionIndex++) {
    var delay = initialDelay + ((switchDelay / sections.length) * sectionIndex);
    console.log('Initialising section ' + sectionIndex + ' with delay ' + delay);
    setTimeout(function() {
      autoSwitchTab(sectionIndex);
    }, delay);
  }
});

