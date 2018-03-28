var express = require('express');
var http = require('http');
var jayson = require('jayson');
var bodyParser = require('body-parser');
var geoip = require('geoip-lite');
var kdt = require("kd.tree")
var net = require('net');

// Create an express app for the RPC Server
var rpcApp = express();

var masterNode; // Placeholder for the static app server
var masterServer; // Store the server object from app.listen
var running = false; // Running state of the static content app
var nodes = []; // List of IP addresses and locs of currently online nodes.
var nodeTree; // KDtree of the nodes

// Set up parsing
rpcApp.use(bodyParser.urlencoded({
  extended: true
}));
rpcApp.use(bodyParser.json());

function checkConnection(node) {
  return new Promise(function(resolve, reject) {
    timeout = 1000; // default of 10 seconds
    var timer = setTimeout(function() {
      resolve({
        success: false,
        error: "timeout"
      });
      socket.end();
    }, timeout);
    var socket = net.createConnection(8080, node.ip, function() {
      clearTimeout(timer);
      resolve({
        success: true,
        node: node
      });
      socket.end();
    });
    socket.on('error', function(err) {
      clearTimeout(timer);
      resolve({
        success: false,
        error: err
      });
    });
  });
}

function buildTree() {
  // Distance function for ndoes
  var distance = function(a, b) {
    return Math.pow(a.lat - b.lat, 2) + Math.pow(a.long - b.long, 2);
  }

  let promiseList = []
  nodes.forEach(function(node) {
    promiseList.push(checkConnection(node, 8080));
  })

  Promise.all(promiseList).then(function(values) {
    onlineNodes = []
    values.forEach(function(node) {
      if (node.success)
        onlineNodes.push(node.node)
    })
    // Rebuild the kdtree
    nodeTree = kdt.createKdTree(onlineNodes, distance, ['lat', 'long'])
  })
}

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

    callback(null, "Set list of nodes")
  }
});

// Add the RPC endpoint
rpcApp.post('/rpc', rpcServer.middleware());

setInterval(buildTree, 2000);

// Export a start function
exports.start = function(app) {
  masterNode = app;
  rpcApp.listen(5000);
}

// Find the closest node
exports.closestNode = function(ip) {
  loc = geoip.lookup(ip)
  if (loc) {
    return nodeTree.nearest({
      lat: loc.ll[0],
      long: loc.ll[1]
    }, 1)[0][0].ip;
  } else {
    return "Error: Could not determine the IP of client";
  }
}
