mod_markdown
============

mod_markdown is Markdown filter module for Apache HTTPD Server.

## Dependencies

* discount

  http://www.pell.portland.or.us/~orc/Code/discount/

## Build and install with APache eXtenSion tool
    $ apxs -i -A -l markdown -c mod_markdown.c

## Configration
in httpd.conf:
```apache
LoadModule markdown_module modules/mod_markdown.so
<Location /markdown>
    AddHandler markdown .md

    # If you want to use stylesheet.
    MarkdownCSS style.css

    # You can also provide an optional argument to 
    # specify media type.
    MarkdownCSS print.css print

    # If you want to include a Google Analytics
    # site id.
    MarkdownGAID UA-12345678-0

</Location>
```
