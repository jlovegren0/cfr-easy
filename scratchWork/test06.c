#include <stdio.h>
#include <curl/curl.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <string.h>

uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
	uint r;
	r = size * nmemb;
	tidyBufAppend(out, in, r);
	return r;
}

char feeCounter, foundFeeSect, foundStaff;
char staffName[] = "REVIEW ENG: &nbsp; &nbsp;";
double totalFees;

void staff_fee(TidyDoc doc, TidyNode tnod, int indent)
{
	TidyNode child;
	for(child = tidyGetChild(tnod); child; child = tidyGetNext(child) ) {
		ctmbstr name = tidyNodeGetName(child);
		if(name)
	   	{
			if ( foundFeeSect )
				feeCounter++;
		}
		else
		{
			TidyBuffer buf;
			tidyBufInit(&buf);
			tidyNodeGetText(doc, child, &buf);
			char * textNode = (char *)buf.bp;
			strtok(textNode,"\n");
			char * staffSection  = strstr(textNode,staffName);
			char * feeSection = strstr(textNode,"FEE:");
			char * teSection = strstr(textNode,"TRACKING ELEMENTS");
			if (staffSection != NULL)
			{
				foundStaff = 1;
		   		printf("%s|", &textNode[strlen(staffName)] );
			}
			if (feeSection != NULL)
				foundFeeSect = 1;
			if (teSection != NULL)
			{
				foundFeeSect = 0;
				if (foundStaff)
					printf("%f\n", totalFees);
				else
					printf("|%f\n",totalFees);
			}
			if (foundFeeSect && (feeCounter > 7) & ((feeCounter - 7) % 9 == 0))
					totalFees += strtod( textNode , NULL );
			//printf("%s\n",textNode);
		}
		staff_fee(doc, child, indent + 4); /* recursive */ 
	}
}

int main(int argc, char** argv)
{
	CURL *curl;
	TidyDoc tdoc;
	TidyBuffer docbuf = {0};
	TidyBuffer tidy_errbuf = {0};
	char curl_errbuf[CURL_ERROR_SIZE];
	int err;
	//
	const char urlString[] = "http://www2.tceq.texas.gov/airperm/index.cfm?"
		"fuseaction=airpermits.project_report&proj_id=%s";
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	char userAgent[] = "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:63.0) Gecko/20100101 Firefox/63.0";
	curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent );
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);
	tdoc = tidyCreate();
	tidyOptSetBool(tdoc, TidyForceOutput, yes);
	tidyOptSetInt(tdoc, TidyWrapLen, 4096);
	for ( int i = 1; i < argc; ++i)
	{
		printf("%s|",argv[i]);
		size_t bufSize = strlen(urlString) + 5;
		char * formedURL = malloc(bufSize * sizeof(char));
		snprintf(formedURL,bufSize,urlString,argv[i]);
		curl_easy_setopt(curl, CURLOPT_URL, formedURL);
		//
		tidySetErrorBuffer(tdoc, &tidy_errbuf);
		tidyBufInit(&docbuf);
		//
		foundFeeSect = 0;
		feeCounter = 0;
		foundStaff = 0;
		totalFees = 0.0;
		//
		err = curl_easy_perform(curl);
		if (!err) {
			err = tidyParseBuffer(tdoc, &docbuf);
			if(err >= 0) {
				err = tidyCleanAndRepair(tdoc);
				if(err >= 0) {
					err = tidyRunDiagnostics(tdoc);
					if(err >= 0) {
						staff_fee(tdoc, tidyGetBody(tdoc), 0);
					}
				}
			}
		}
		tidyBufFree(&docbuf);
	}
		//tidyBufFree(&tidy_errbuf);
		tidyRelease(tdoc);
	curl_easy_cleanup(curl);
	return(0);
}
