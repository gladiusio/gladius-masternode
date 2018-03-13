var express = require('express');
var jayson = require('jayson');
var bodyParser = require('body-parser');
var geoip = require('geoip-lite');
var kdt = require("kd.tree")

// Create an express app for the RPC Server
var rpcApp = express();

var masterNode; // Placeholder for the static app server
var masterServer; // Store the server object from app.listen
var running = false; // Running state of the static content app
var nodes = []; // List of IP addresses and locs of currently online nodes.
var nodeTree;

// Set up parsing
rpcApp.use(bodyParser.urlencoded({
  extended: true
}));
rpcApp.use(bodyParser.json());

// Build Jayson (JSON-RPC server)
var rpcServer = jayson.server({
  start: function(args, callback) {
    masterServer = masterNode.listen(8080); // Start the app
    running = true;
    callback(null, "Started server");
  },
  stop: function(args, callback) {
    if (staticServer) { // Make sure the server is started
      masterServer.close(); // Stop the app
      running = false;
      callback(null, "Stopped server");
    } else {
      callback(null, "Server was not running so can't be stopped.");
    }
  },
  status: function(args, callback) {
    callback(null, {
      running: running // Return the current running status
    })
  },
  loadNodes: function(args, callback) {
    nodes = args.map(function(ip) {
      ll = geoip.lookup(ip).ll;
      return {
        lat: ll[0],
        long: ll[1],
        ip: ip
      };
    })

    // Distance function for ndoes
    var distance = function(a, b) {
      return Math.pow(a.lat - b.lat, 2) + Math.pow(a.long - b.long, 2);
    }

    // Rebuild the kdtree
    nodeTree = kdt.createKdTree(nodes, distance, ['lat', 'long'])

    callback(null, "Added new nodes and rebuilt kdtree")
  }
});

// Add the RPC endpoint
rpcApp.post('/rpc', rpcServer.middleware());

// Export a start function
exports.start = function(app) {
  masterNode = app;
  rpcApp.listen(5000);
}

// Find the closest node
exports.closestNode = function(ip) {
  ll = geoip.lookup(ip).ll;
  return nodeTree.nearest({
    lat: ll[0],
    long: ll[1]
  }, 1);
}
