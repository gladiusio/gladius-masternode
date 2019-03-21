#include "ServiceWorker.h"
#include <myhtml/api.h>

#include <folly/FileUtil.h>

ServiceWorker::ServiceWorker(const std::string& path) {
    if (!folly::readFile(path.c_str(), payload_)) {
        LOG(FATAL) << 
            "Could not read service worker javascript file: " << path;
    }
}

// todo: optimize use of threads with the myhtml library
std::string ServiceWorker::injectServiceWorker(
    folly::IOBuf buf) const {
    myhtml_t* myhtml = myhtml_create(); // create instance of myhtml
    myhtml_init(myhtml, MyHTML_OPTIONS_DEFAULT, 1, 0);
    myhtml_tree_t* tree = myhtml_tree_create();
    myhtml_tree_init(tree, myhtml);
    // ingest cached content
    mystatus_t status = myhtml_parse(tree, MyENCODING_UTF_8, 
        reinterpret_cast<const char*>(buf.data()), buf.length());
    if (status != MyHTML_STATUS_OK) {
        VLOG(1) << "Could not parse cached HTML content";
        return "";
    }
    // find the head tag
    myhtml_tree_node_t* head = myhtml_tree_get_node_head(tree);

    if (!head) {
        VLOG(1) << "Could not parse <head>";
        return "";
    }
    // insert service worker script tag
    myhtml_tree_node_t* sw_tag = 
        myhtml_node_create(tree, MyHTML_TAG_SCRIPT, MyHTML_NAMESPACE_HTML);
    myhtml_tree_node_t* text = 
        myhtml_node_create(tree, MyHTML_TAG__TEXT, MyHTML_NAMESPACE_HTML);
    myhtml_node_text_set(text, inject_script_.c_str(),
        inject_script_.length(), MyENCODING_UTF_8);
    // add the injection code into the <script> tag
    myhtml_node_append_child(sw_tag, text);
    // add the <script> tag into the <head> section
    myhtml_node_append_child(head, sw_tag);

    mycore_string_raw_t str;
    mycore_string_raw_clean_all(&str);
    myhtml_serialization_tree_buffer(myhtml_tree_get_document(tree), &str);

    std::string injected(str.data, str.length);
    
    mycore_string_raw_destroy(&str, false);
    myhtml_tree_destroy(tree);
    myhtml_destroy(myhtml);
    
    return injected;
}

std::string ServiceWorker::getPayload() const { return payload_; }
std::string ServiceWorker::getInjectScript() const { return inject_script_; }