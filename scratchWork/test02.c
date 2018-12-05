#include <stdio.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <curl/curl.h>

uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
	uint r;
	r = size * nmemb;
	tidyBufAppend(out, in, r);
	return r;
}

void dumpNode(TidyDoc doc, TidyNode tnod, int indent)
{
	TidyNode child;
	for(child = tidyGetChild(tnod); child; child = tidyGetNext(child) ) {
		ctmbstr name = tidyNodeGetName(child);
		if(name) {
			TidyAttr attr;
			printf("%*.*s%s ", indent, indent, "<", name);
			for(attr = tidyAttrFirst(child); attr; attr = tidyAttrNext(attr) ) {
				printf("%s",tidyAttrName(attr));
				tidyAttrValue(attr)?printf("=\"%s\" ",
						tidyAttrValue(attr)):printf(" ");
			}
			printf(">\n");
		}
		else {
			TidyBuffer buf;
			tidyBufInit(&buf);
			tidyNodeGetText(doc, child, &buf);
			printf("%*.*s\n", indent, indent, buf.bp?(char *)buf.bp:"");
			tidyBufFree(&buf);
		}
		dumpNode(doc, child, indent + 4);
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
	char urlString[1024];
	strcpy(urlString , "444");
	strcat(urlString , argv[1] );
	printf("%s\n",urlString);
	if(argc == 2) {
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

		tdoc = tidyCreate();
		tidyOptSetBool(tdoc, TidyForceOutput, yes);
		tidyOptSetInt(tdoc, TidyWrapLen, 4096);
		tidySetErrorBuffer(tdoc, &tidy_errbuf);
		tidyBufInit(&docbuf);

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);
		err = curl_easy_perform(curl);
		if(!err) {
			err = tidyParseBuffer(tdoc, &docbuf);
			if(err >= 0) {
				err = tidyCleanAndRepair(tdoc);
				if(err >= 0) {
					err = tidyRunDiagnostics(tdoc);
					if(err >= 0) {
						dumpNode(tdoc, tidyGetRoot(tdoc), 0);
						fprintf(stderr, "%s\n", tidy_errbuf.bp);
					}
				}
			}
		}
		else
			fprintf(stderr, "%s\n", curl_errbuf);

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
