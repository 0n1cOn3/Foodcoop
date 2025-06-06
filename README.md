# Foodcoop

This example Qt6 application fetches daily prices from Swiss grocery stores,
stores them in a SQLite database and plots the price history. A simple trend
detection is performed using linear regression on the stored prices.

The `PriceFetcher` class mimics a desktop browser when scraping the Swiss stores
(Coop, Migros, Denner, Aldi Suisse, Lidl Suisse and Ottos Warenposten). When a site requires JavaScript, a headless QtWebEngine page is used automatically. Product links are resolved
dynamically by searching the site before fetching the product page. The resolved
URL is stored in the SQLite database so the application can detect when a store
moves a product and update the saved link automatically. The scraping code is
intentionally simple and may require adjustments when the page markup changes.
Requests include an `Accept-Language` header for Swiss German (`de-CH`) so
search results match the German product names. If Qt WebEngine is not
available the application still builds, but any pages that require JavaScript
will fail to load and an issue is recorded.

If you do not have network access, set the environment variable `OFFLINE_PATH`
to a directory containing HTML files saved from Firefox. The filenames do not
need to follow a strict pattern – the application matches files by searching
for the store and item names inside the page title. A typical `page title •
Store.html` created by Firefox will work. In this mode the pages are read from
disk instead of downloaded.

When you choose an offline folder in the application settings the program
now scans all HTML files in that directory. Basic information such as the
product name, price, country of origin and weight is extracted and printed
to the console.

## Prerequisites

Install the Qt6 development modules required to build the application. On
Debian or Ubuntu systems you can install them using `apt`:

```bash
sudo apt-get install qt6-base-dev qt6-webengine-dev \
    qt6-charts-dev qt6-sql-dev
```

If you use another operating system, install the same packages with your
distribution's package manager.

## Building

```
mkdir build
cd build
cmake ..
make
```

Run the application with `./Foodcoop`.

On the very first start the program shows a small progress dialog while it
scrapes every configured store. A cancel button allows aborting the initial
scrape and a toggle button reveals a debug log of the scraping process. The
main window only appears once each store has produced at least one price entry. On
later runs prices are fetched silently in the background.

New products can be added through the text field on the left side menu. The app
will store the product URL for each store and automatically keep it updated when
it changes.

Any errors encountered while scraping are saved in an `issues` table and shown
in the **Issues** tab so you can inspect problems with individual requests.
