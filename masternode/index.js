var express = require('express');
var geoip = require('geoip-lite');
var rpcControl = require('./controller/manager');

app = express();

app.set('view engine', 'pug');
app.set('views', './views');

// Create a demo route
app.get('/', function(req, res) {
  var clientAddress = req.headers['x-forwarded-for']; // Pull the client IP (behind NGINX only)
  var edgeAddress = rpcControl.closestNode(clientAddress); // Get closest node
  console.log(edgeAddress);

  // Build a demo page with content from edge nodes
  res.render('index.pug', {
    title: 'Gladius Demo',
    message: 'Woah a message',
    clientAddress: clientAddress,
    edgeAddress: edgeAddress
  })
})

rpc = rpcControl.start(app);
