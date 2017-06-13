var page = require('webpage').create();
var system = require('system');
var args = system.args;

//viewportSize being the actual size of the headless browser
//page.viewportSize = { width: 1024, height: 768 };
//the clipRect is the portion of the page you are taking a screenshot of
//page.clipRect = { top: 0, left: 0, width: 1024, height: 768 };

page.open(args[1], function(status) {
    args.forEach(function(arg, i) {
        console.log(i + ': ' + arg);
    });
    if (status !== 'success') {
        console.log('Unable to load the address!');
        phantom.exit();
    } else {
        window.setTimeout(function() {
            page.render(args[2]);
            phantom.exit();
        }, 1000); // Change timeout as required to allow sufficient time
    }
});
