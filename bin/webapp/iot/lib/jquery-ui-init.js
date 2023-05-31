$(function() {
  $(".nav_element").button().click(function(event) {
    event.preventDefault();
  });
  $(".nav_element").addClass("ui-button-lj");
});