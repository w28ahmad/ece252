#include "utils.h"

htmlDocPtr mem_getdoc(char *buf, int size, const char *url)
{
	int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | \
			   HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
	htmlDocPtr doc = htmlReadMemory(buf, size, url, NULL, opts);

	if ( doc == NULL ) {
		fprintf(stderr, "Document not parsed successfully.\n");
		return NULL;
	}
	return doc;
}

xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath)
{

	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	context = xmlXPathNewContext(doc);
	if (context == NULL) {
		printf("Error in xmlXPathNewContext\n");
		return NULL;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		printf("Error in xmlXPathEvalExpression\n");
		return NULL;
	}
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
		//printf("No result\n");
		return NULL;
	}
	return result;
}

int find_http(char *buf, int size, int follow_relative_links, const char *base_url)
{

	int i;
	htmlDocPtr doc;
	xmlChar *xpath = (xmlChar*) "//a/@href";
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr result;
	xmlChar *href;

	if (buf == NULL) {
		return 1;
	}

	doc = mem_getdoc(buf, size, base_url);
	result = getnodeset (doc, xpath);
	if (result) {
		nodeset = result->nodesetval;
		for (i=0; i < nodeset->nodeNr; i++) {
			href = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
			if ( follow_relative_links ) {
				xmlChar *old = href;
				href = xmlBuildURI(href, (xmlChar *) base_url);
				xmlFree(old);
			}
			if ( href != NULL && !strncmp((const char *)href, "http", 4) ) {
				/* printf("href: %s\n", href); */
				/* Add these url's right to the hashmap */
				add_url_to_map((char*)href);
			}
			xmlFree(href);
		}
		xmlXPathFreeObject (result);
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();
	return 0;
}
