/*
**  mod_markdown.c -- Apache sample markdown module
**  [Autogenerated via ``apxs -n markdown -g'']
**
**  To play with this sample module first compile it into a
**  DSO file and install it into Apache's modules directory
**  by running:
**
**    $ apxs -c -i mod_markdown.c
**
**  Then activate it in Apache's httpd.conf file for instance
**  for the URL /markdown in as follows:
**
**    #   httpd.conf
**    LoadModule markdown_module modules/mod_markdown.so
**    <Location /markdown>
**    AddHandler markdown .md
**    </Location>
**
**  Then after restarting Apache via
**
**    $ apachectl restart
**
**  you immediately can request the URL /markdown and watch for the
**  output of this module. This can be achieved for instance via:
**
**    $ lynx -mime_header http://localhost/markdown
**
**  The output should be similar to the following one:
**
**    HTTP/1.1 200 OK
**    Date: Tue, 31 Mar 1998 14:42:22 GMT
**    Server: Apache/1.3.4 (Unix)
**    Connection: close
**    Content-Type: text/html
**
**    The sample page from mod_markdown.c
*/

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_log.h"
#include "ap_config.h"

#include "mkdio.h"

module AP_MODULE_DECLARE_DATA markdown_module;

typedef struct {
    const void *path;
    const void *media;
    struct list_t *next;
} list_t;

typedef struct {
    list_t *css;
    const char *header;
    const char *footer;
    const char *ga_id;
} markdown_conf;

#define P(s) ap_rputs(s, r)

void markdown_output(MMIOT *doc, request_rec *r)
{
    char *title;
    char *author;
    char *date;
    int ret;
    int size;
    char *p;
    markdown_conf *conf;
    list_t *css;

    conf = (markdown_conf *) ap_get_module_config(r->per_dir_config,
                                                  &markdown_module);
    ret = mkd_compile(doc, MKD_AUTOLINK);
    ap_rputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", r);
    ap_rputs("<!DOCTYPE html PUBLIC \n"
             "          \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
             "          \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n",
             r);
    ap_rputs("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n", r);
    ap_rputs("<head>\n", r);

    author = mkd_doc_author(doc);
    if (author) {
        ap_rprintf(r,
                   "<meta name=\"author\" content=\"%s\" />\n",
                   (char *) author);
    }

    date = mkd_doc_date(doc);
    if (date) {
        ap_rprintf(r,
                   "<meta name=\"date\" content=\"%s\" />\n",
                   (char *) date);
    }

    if (conf->css) {
        ap_rputs("<meta http-equiv=\"Content-Type\""
                 " content=\"text/html; charset=UTF-8\" />\n", r);
        ap_rputs("<meta http-equiv=\"Content-Style-Type\""
                 " content=\"text/css\" />\n", r);
		css = conf->css;
		do{
            if (css->media == NULL) {
                ap_rprintf(r,
                           "<link rel=\"stylesheet\" href=\"%s\""
                           " type=\"text/css\" />\n",
                           (char *)css->path);
            }
            else {
                ap_rprintf(r,
                           "<link rel=\"stylesheet\" href=\"%s\""
                           " type=\"text/css\" media=\"%s\" />\n",
                           (char *)css->path,
                           (char *)css->media);
            }
            css = (list_t *)css->next;
		}while(css);
    }
    title = mkd_doc_title(doc);
    if (title) {
        ap_rprintf(r, "<title>%s</title>\n", title);
    }

    if (conf->ga_id) {
        ap_rputs("<script type=\"text/javascript\">\n", r);
        //ap_rputs("<!CDATA[\n", r);
        ap_rputs("var _gaq = _gaq || [];\n", r);
        ap_rprintf(r,
                   "_gaq.push(['_setAccount', '%s'])\n",
                   (char *)conf->ga_id);
        ap_rputs("_gaq.push(['_trackPageview']);\n", r);
        ap_rputs("(function() {\n", r);
        ap_rputs("var ga = document.createElement('script');"
                 " ga.type = 'text/javascript'; ga.async = true;\n", r);
        ap_rputs("ga.src = ('https:' == document.location.protocol ?"
                 " 'https://ssl' : 'http://www') +"
                 " '.google-analytics.com/ga.js';\n", r);
        ap_rputs("var s = document.getElementsByTagName('script')[0];"
                 " s.parentNode.insertBefore(ga, s);\n", r);
        ap_rputs("})();\n", r);
        //ap_rputs("]]>\n", r);
        ap_rputs("</script>\n", r);
    }

    ap_rputs("</head>\n", r);
    ap_rputs("<body>\n", r);
    if ((size = mkd_document(doc, &p)) != EOF) {
        ap_rwrite(p, size, r);
    }
    ap_rputc('\n', r);
    ap_rputs("</body>\n", r);
    ap_rputs("</html>\n", r);
    mkd_cleanup(doc);
}

void raw_output(FILE * fp, request_rec * r)
{
    char buf[1024];
    size_t len;
    while (1) {
        len = fread(buf, 1, 1024, fp);
        if (len <= 0) {
            break;
        }
        ap_rwrite(buf, len, r);
    }
}

/* The markdown handler */
static int markdown_handler(request_rec *r)
{
    FILE *fp;
    MMIOT *doc;

    if (strcmp(r->handler, "markdown")) {
        return DECLINED;
    }

    if (r->header_only) {
        return OK;
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "markdown_handler(): %s", r->filename);

    fp = fopen(r->filename, "r");
    if (fp == NULL) {
        switch (errno) {
        case ENOENT:
            return HTTP_NOT_FOUND;
        case EACCES:
            return HTTP_FORBIDDEN;
        default:
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "open error, errno: %d\n", errno);
            return HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    if (r->args && !strcasecmp(r->args, "raw")) {
        r->content_type = "text/plain; charset=UTF-8";
        raw_output(fp, r);
        fclose(fp);
    } else {
        r->content_type = "application/xhtml+xml";
        doc = mkd_in(fp, 0);
        fclose(fp);
        if (doc == NULL) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "mkd_in() returned NULL\n");
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        markdown_output(doc, r);
    }

    return OK;
}



static void *markdown_config(apr_pool_t * p, char *dummy)
{
    markdown_conf *c =
        (markdown_conf *) apr_pcalloc(p, sizeof(markdown_conf));
    memset(c, 0, sizeof(markdown_conf));
    return (void *) c;
}

static const char *set_markdown_css(cmd_parms * cmd, void *conf,
                                    const char *path,
                                    const char *media)
{
    markdown_conf *c = (markdown_conf *) conf;
    list_t *item = (list_t *)malloc(sizeof(list_t));
    item->path = path;
    if (media == NULL) {
        item->media = NULL;
    }
    else {
        item->media = media;
    }
    item->next = NULL;

    list_t *tail;
    if(c->css){
        tail = c->css;
        while(tail->next) tail = (list_t *)tail->next;
        tail->next = (struct list_t *)item;
    }else{
        c->css = item;
    }
    return NULL;
}

static const char *set_markdown_header(cmd_parms * cmd, void *conf,
									   const char *arg)
{
    markdown_conf *c = (markdown_conf *) conf;
    c->header = arg;
    return NULL;
}

static const char *set_markdown_footer(cmd_parms * cmd, void *conf,
									   const char *arg)
{
    markdown_conf *c = (markdown_conf *) conf;
    c->footer = arg;
    return NULL;
}

static const char *set_markdown_ga_id(cmd_parms * cmd, void *conf,
									   const char *arg)
{
    markdown_conf *c = (markdown_conf *) conf;
    c->ga_id = arg;
    return NULL;
}

static const command_rec markdown_cmds[] = {
    AP_INIT_TAKE12("MarkdownCSS", set_markdown_css, NULL, OR_ALL,
                  "set CSS"),
    AP_INIT_TAKE1("MarkdownHeaderHtml", set_markdown_footer, NULL, OR_ALL,
                  "set Header HTML"),
    AP_INIT_TAKE1("MarkdownFooterHtml", set_markdown_footer, NULL, OR_ALL,
                  "set Footer HTML"),
    AP_INIT_TAKE1("MarkdownGAID", set_markdown_ga_id, NULL, OR_ALL,
                  "set Google Analytics token"),
    {NULL}
};

static void markdown_register_hooks(apr_pool_t * p)
{
    ap_hook_handler(markdown_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA markdown_module = {
    STANDARD20_MODULE_STUFF,
    markdown_config,            /* create per-dir    config structures */
    NULL,                       /* merge  per-dir    config structures */
    NULL,                       /* create per-server config structures */
    NULL,                       /* merge  per-server config structures */
    markdown_cmds,              /* table of config file commands       */
    markdown_register_hooks     /* register hooks                      */
};
