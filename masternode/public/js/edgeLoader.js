function httpGet(theUrl) {
  let xmlHttp = new XMLHttpRequest();
  xmlHttp.open("GET", theUrl, false); // false for synchronous request
  xmlHttp.send(null);
  return xmlHttp.responseText;
}

function fetchAndLoadEdge() {
  let masterInfo = JSON.parse(httpGet("/master_info")); // Information about node and files
  let bundleString = httpGet(masterInfo.bundle);

  // Verify the file matches
  console.log(md5(bundleString).toUpperCase());
  console.log(masterInfo.expectedHash.toUpperCase());

  if (md5(bundleString).toUpperCase() === masterInfo.expectedHash.toUpperCase()) {
    let edgeData = JSON.parse(bundleString);

    for (let dataKey of Object.keys(edgeData)) {
      let lookup = {
        js: {
          tag: 'script',
          type: 'text',
          format: 'javascript'
        },
        jpg: {
          tag: 'img',
          type: 'image',
          format: 'jpg'
        },
        png: {
          tag: 'img',
          type: 'image',
          format: 'png'
        },
        css: {
          tag: 'link'
        }
      }

      let split = dataKey.split(".");
      let extension = split[split.length - 1];
      let tagType = "";
      let srcString = "";

      // Create a tag and attach the data
      let tag = document.createElement(lookup[extension]['tag']);
      if (extension === "css") {
        srcString = "data:text/css;base64," + +edgeData[dataKey];
        tag.href = srcString;
        tag.rel = 'stylesheet'
      } else {
        srcString = "data:" + lookup[extension]['type'] + "/" + lookup[
          extension]['format'] + ";base64," + edgeData[dataKey];
        tag.src = srcString;
      }

      // Insert the tag as a child to it's div
      document.getElementById(dataKey).appendChild(tag);

    }
  } else {
    alert("Edge node delivered non matching content!")
  }
}

fetchAndLoadEdge();
