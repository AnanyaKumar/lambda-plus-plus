;(function($) {

  // Determine if an element is still visible in the viewport
  function isVisible(el) {
    var rect = el.getBoundingClientRect();

    return rect.top >= 150;
  }

  // Set up listeners to toggle opacity of header (fade in and out)
  function initFadingHeader () {
    // modified from <http://stackoverflow.com/questions/123999/>

    var floating = $('.floating-header');
    var contentPanel = $('.content-wrapper');

    var didScroll = true;

    function hasScrolled() {
      if (isVisible(contentPanel) && floating.classList.contains('visible')) {
        floating.classList.remove('visible');
      }
      else if (!isVisible(contentPanel) && !floating.classList.contains('visible')) {
        floating.classList.add('visible');
      }
    }

    setInterval(function () {
      if (didScroll) {
        hasScrolled();
        didScroll = false;
      }
    }, 250);

    function listener(e) {
      didScroll = true;
    }

    if (window.addEventListener) {
      addEventListener('DOMContentLoaded', listener, false);
      addEventListener('load', listener, false);
      addEventListener('scroll', listener, false);
      addEventListener('resize', listener, false);
    } else if (window.attachEvent)  {
      attachEvent('onDOMContentLoaded', listener); // IE9+ :(
      attachEvent('onload', listener);
      attachEvent('onscroll', listener);
      attachEvent('onresize', listener);
    }
  }

  // ----- main -----
  document.addEventListener("DOMContentLoaded", function() {

    // Statically determine height of main hero once page has loaded
    // (cirvumvents mobile weirdness)
    $('.hero').style.minHeight = "" + window.innerHeight + "px";

    initFadingHeader();
  });

})(document.querySelector.bind(document));

