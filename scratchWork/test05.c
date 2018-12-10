#include <stdio.h>
#include <curl/curl.h>
#include <tidy.h>
#include <tidybuffio.h>

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
char tripWire = 0;
char feeCounter = 0;

/* Traverse the document tree */ 
void dumpNode(TidyDoc doc, TidyNode tnod, int indent)
{
	char level = indent/4;
	TidyNode child;
	for(child = tidyGetChild(tnod); child; child = tidyGetNext(child) ) {
		ctmbstr name = tidyNodeGetName(child);
		if(name) {
			if ( tripWire )
				feeCounter++;
			/* if it has a name, then it's an HTML tag ... */ 
			TidyAttr attr;
			//printf("%*.*s%s ", indent, indent, "<", name);
			if( (switch0 > 0) && (strcmp(name,"tr") == 0) )
				//printf("\n");
			/* walk the attribute list */ 
			for(attr = tidyAttrFirst(child); attr; attr = tidyAttrNext(attr) ) {
				//printf("%s",tidyAttrName(attr));
				//tidyAttrValue(attr)?printf("=\"%s\" ", tidyAttrValue(attr)):printf(" ");
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
			//printf("(%i)%s\n",strcmp(textNode,"REVIEW ENG:"),textNode);
			char staffName[] = "REVIEW ENG: &nbsp; &nbsp;";
			char * offset  = strstr(textNode,staffName);
			char * offset2 = strstr(textNode,"FEE:");
			char * offset3 = strstr(textNode,"TRACKING ELEMENTS");
			if (offset != NULL)
			{
				printf("%s\n",&textNode[strlen(staffName)]);
			}
			if (offset2 != NULL)
				tripWire = 1;
			if (offset3 != NULL)
				tripWire = 0;
			if (tripWire && (feeCounter > 7) & ((feeCounter - 7) % 9 == 0))
					printf("%s\n",textNode);
			//if (switch0 > 0)
				//printf( "%s|",  (char *)buf.bp );
			tidyBufFree(&buf);
		}
		dumpNode(doc, child, indent + 4); /* recursive */ 
		}
}

int main(int argc, char **argv)
{
	CURL *curl;
for ( char i = 1; i < argc; ++i)
	{
	char curl_errbuf[CURL_ERROR_SIZE];
	TidyDoc tdoc;
	TidyBuffer docbuf = {0};
	TidyBuffer tidy_errbuf = {0};
	int err;
	char* urlString = "http://www2.tceq.texas.gov/airperm/index.cfm";
	char* postData = 
	"fuseaction=airpermits.project_report&"
	"proj_id=%s&";
	char postData2[strlen(postData)+4];
	sprintf(postData2,postData,argv[i]);
	char** hdrs = malloc(sizeof(*hdrs)*4);
   	hdrs[0] = "Connection: keep-alive";
    hdrs[1] = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
    hdrs[2] = "Upgrade-Insecure-Requests: 1";
	hdrs[3] = "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:63.0) Gecko/20100101 Firefox/63.0";
	struct curl_slist *list = NULL;
	list = curl_slist_append(list, hdrs[0] );
	list = curl_slist_append(list, hdrs[1] );
	list = curl_slist_append(list, hdrs[2] );
	if(1) {
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, urlString);
		curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,strlen(postData2));
		curl_easy_setopt(curl,CURLOPT_POSTFIELDS,postData2);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, hdrs[3] );
		curl_easy_setopt(curl, CURLOPT_REFERER, hdrs[4] );
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list );
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
		//curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

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
						//fprintf(stderr, "%s\n", tidy_errbuf.bp);
					}
				}
			}
		}
		else
			fprintf(stderr,"%s\n",curl_errbuf);

		curl_slist_free_all(list);
		tidyBufFree(&docbuf);
		tidyBufFree(&tidy_errbuf);
		tidyRelease(tdoc);
		//return err;

	}
	//else
		//printf("usage: %s <url>\n", argv[0]);
//
	//return 0;
	}
		curl_easy_cleanup(curl);
}
