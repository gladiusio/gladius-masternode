function httpGet(theUrl) {
  let xmlHttp = new XMLHttpRequest();
  xmlHttp.open("GET", theUrl, false); // false for synchronous request
  xmlHttp.send(null);
  return xmlHttp.responseText;
}

function fetchAndLoadEdge() {
  fetch("/master_info").then(function(response) {
      return response.json();
    })
    .then(function(masterInfo) {
      fetch(masterInfo.bundle).then(function(response) {
        return response.text();
      }).then(function(bundleString) {
        // Show hashes
        document.getElementById("actual_hash").innerHTML = sha256(
          bundleString).toUpperCase();
        document.getElementById("expected_hash").innerHTML = masterInfo.expectedHash
          .toUpperCase();

        if (sha256(bundleString).toUpperCase() === masterInfo.expectedHash
          .toUpperCase()) {
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

            // Get some info about the tag
            let split = dataKey.split(".");
            let extension = split[split.length - 1];
            let tagType = "";
            let srcString = "";

            // Create a tag and attach the data
            let tag = document.createElement(lookup[extension]['tag']);
            // Get parent tag
            let parent = document.getElementById(dataKey);

            if (extension === "css") {
              srcString = "data:text/css;base64," + edgeData[dataKey];
              tag.href = srcString;
              tag.rel = 'stylesheet'
            } else {
              srcString = "data:" + lookup[extension]['type'] + "/" +
                lookup[
                  extension]['format'] + ";base64," + edgeData[dataKey];
              tag.src = srcString;
              // Make sure that this script executes before others load.
              if (lookup[extension]['tag'] === 'script')
                tag.async = false;
            }

            tag.id = dataKey;

            // Replace the existing tag with the new one.
            parent.innerHTML = tag.inerHTML;
            parent.parentNode.replaceChild(tag, parent);

          }
        } else {
          alert("Edge node delivered non matching content!")
        }
      });
    });


}

fetchAndLoadEdge();
