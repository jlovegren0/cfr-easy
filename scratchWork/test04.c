#include <stdio.h>
#include <curl/curl.h>

#include <tidybuffio.h>
#include <curl/curl.h>

#define THRESH_INIT 127
 
/* curl write callback, to fill tidy's input buffer...  */ 
uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
	uint r;
	r = size * nmemb;
	tidyBufAppend(out, in, r);
	return r;
}
 
char switch0 = 0;
char levelThresh = THRESH_INIT;
char lastLevel = 0;
/* Traverse the document tree */ 
void dumpNode(TidyDoc doc, TidyNode tnod, int indent)
{
	char level = indent/4;
	TidyNode child;
	for(child = tidyGetChild(tnod); child; child = tidyGetNext(child) ) {
		ctmbstr name = tidyNodeGetName(child);
		if(name) {
			/* if it has a name, then it's an HTML tag ... */ 
			TidyAttr attr;
			//printf("%i%*.*s%s ",levelThresh, indent, indent, "<", name);
			if( (switch0 > 0) && (strcmp(name,"tr") == 0) )
				printf("\n");
			/* walk the attribute list */ 
			for(attr = tidyAttrFirst(child); attr; attr = tidyAttrNext(attr) ) {
				//printf("%s",tidyAttrName(attr));
				//tidyAttrValue(attr)?printf("=\"%s\" ", tidyAttrValue(attr)):printf(" ");
				if ( strcmp(tidyAttrValue(attr),"display1" ) == 0 )
				{
					switch0 = 1;
					levelThresh = level;
				}
				else if ( (switch0 == 1) && (level < levelThresh) )
					switch0 = 0;
			}
			//printf(">\n");
		}
		else {
		/* if it doesn't have a name, then it's probably text, cdata, etc... */ 
			TidyBuffer buf;
			tidyBufInit(&buf);
			tidyNodeGetText(doc, child, &buf);
			char * textNode = (char *)buf.bp;
			strtok(textNode,"\n");
			//printf("%*.*s\n", indent, indent, buf.bp?(char *)buf.bp:"");
			if (switch0 > 0)
				printf( "%s|",  (char *)buf.bp );
			tidyBufFree(&buf);
		}
		lastLevel = level;
		dumpNode(doc, child, indent + 4); /* recursive */ 
		}
}

int main(int argc, char **argv)
{
	CURL *curl;
	char curl_errbuf[CURL_ERROR_SIZE];
	TidyDoc tdoc;
	TidyBuffer docbuf = {0};
	TidyBuffer tidy_errbuf = {0};
	int err;
	char* urlString = "http://www2.tceq.texas.gov/airperm/index.cfm";
	char* postData = "RequestTimeout=3000&"
	"fuseaction=searchprojects&"
	"loc_cnty_name=HARRIS&"
	"proj_id=&"
	"tnrcc_region_cd=0&"
	"addn_num_txt=&"
	"addn_id_typ_txt=&"
	"cn_issue_to_txt=&"
	"proj_typ_txt=&"
	"cn_ref_num_txt=&"
	"account=&"
	"rn_ref_num_txt=&"
	"proj_status_txt=ALL&"
	"date_option=0&"
	"date_range_from=&"
	"date_range_to=&"
	"sort_dir=asc&"
	"program=NSR&"
	"order_by=proj.proj_id&"
	"out_form=web&"
	"_fuseaction%3Dairpermits.validate_search_criteria.x=17&"
	"_fuseaction%3Dairpermits.validate_search_criteria.y=10";
	char** hdrs = malloc(sizeof(*hdrs)*5);
   	hdrs[0] = "Connection: keep-alive";
    hdrs[1] = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
    hdrs[2] = "Upgrade-Insecure-Requests: 1";
	hdrs[3] = "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:63.0) Gecko/20100101 Firefox/63.0";
	hdrs[4] = "http://www2.tceq.texas.gov/airperm/index.cfm?fuseaction=airpermits.start";
	struct curl_slist *list = NULL;
	list = curl_slist_append(list, hdrs[0] );
	list = curl_slist_append(list, hdrs[1] );
	list = curl_slist_append(list, hdrs[2] );
	if(argc == 1) {
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, urlString);
		curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,strlen(postData));
		curl_easy_setopt(curl,CURLOPT_POSTFIELDS,postData);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, hdrs[3] );
		curl_easy_setopt(curl, CURLOPT_REFERER, hdrs[4] );
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list );
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

		tdoc = tidyCreate();
		tidyOptSetBool(tdoc, TidyForceOutput, yes);
		tidyOptSetInt(tdoc, TidyWrapLen, 4096);
		tidySetErrorBuffer(tdoc, &tidy_errbuf);
		tidyBufInit(&docbuf);

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);
		err = curl_easy_perform(curl);
		if (!err) {
			err = tidyParseBuffer(tdoc, &docbuf);
			if(err >= 0) {
				err = tidyCleanAndRepair(tdoc);
				if(err >= 0) {
					err = tidyRunDiagnostics(tdoc);
					if(err >= 0) {
						dumpNode(tdoc, tidyGetBody(tdoc), 0);
						fprintf(stderr, "%s\n", tidy_errbuf.bp);
					}
				}
			}
		}
		else
			fprintf(stderr,"%s\n",curl_errbuf);

		curl_slist_free_all(list);
		curl_easy_cleanup(curl);
		tidyBufFree(&docbuf);
		tidyBufFree(&tidy_errbuf);
		tidyRelease(tdoc);
		return err;

	}
	else
		printf("usage: %s <url>\n", argv[0]);

	return 0;
}
