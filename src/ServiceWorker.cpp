#include "ServiceWorker.h"
#include <myhtml/api.h>

#include <folly/FileUtil.h>

ServiceWorker::ServiceWorker(std::string path) {
    if (!folly::readFile(path.c_str(), payload_)) {
        LOG(ERROR) << "Could not read service worker javascript file: " << path;
    }
    LOG(INFO) << "Service worker content: " << payload_;
}

// todo: optimize use of threads with the myhtml library
folly::fbstring ServiceWorker::injectServiceWorker(folly::IOBuf buf) {
    myhtml_t* myhtml = myhtml_create(); // create instance of myhtml
    myhtml_init(myhtml, MyHTML_OPTIONS_DEFAULT, 1, 0);
    myhtml_tree_t* tree = myhtml_tree_create();
    myhtml_tree_init(tree, myhtml);
    mystatus_t status = myhtml_parse(tree, MyENCODING_UTF_8, 
        reinterpret_cast<const char*>(buf.data()), buf.length()); // ingest cached content
    if (status != MyHTML_STATUS_OK) {
        LOG(ERROR) << "Could not parse cached HTML content";
        return "";
    }
    // find the head tag
    myhtml_tree_node_t* head = myhtml_tree_get_node_head(tree);

    if (!head) {
        LOG(ERROR) << "Could not parse <head>";
        return "";
    }
    // insert service worker script tag
    myhtml_tree_node_t* sw_tag = myhtml_node_create(tree, MyHTML_TAG_SCRIPT, MyHTML_NAMESPACE_HTML);
    myhtml_tree_node_t* text = myhtml_node_create(tree, MyHTML_TAG__TEXT, MyHTML_NAMESPACE_HTML);
    myhtml_node_text_set(text, inject_script_.c_str(),
        inject_script_.length(), MyENCODING_UTF_8);
    myhtml_node_append_child(sw_tag, text); // add the injection code into the <script> tag
    myhtml_node_append_child(head, sw_tag); // add the <script> tag into the <head> section

    mycore_string_raw_t str;
    mycore_string_raw_clean_all(&str);
    myhtml_serialization_tree_buffer(myhtml_tree_get_document(tree), &str);

    folly::fbstring injected(str.data, str.length);
    
    mycore_string_raw_destroy(&str, false);
    myhtml_tree_destroy(tree);
    myhtml_destroy(myhtml);
    
    return injected;
}

folly::fbstring ServiceWorker::getPayload() { return payload_; }
folly::fbstring ServiceWorker::getInjectScript() { return inject_script_; }