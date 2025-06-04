# Foodcoop Price Tracker

This example Qt6 application fetches daily prices from Swiss grocery stores,
stores them in a SQLite database and plots the price history. A simple trend
detection is performed using linear regression on the stored prices.

The `PriceFetcher` class now mimics a desktop browser when requesting product
pages from Coop, Migros, Denner, Aldi Suisse and Lidl Suisse. Because the
stores do not provide open APIs, the price is extracted from the HTML using a
simple regular expression. The included URLs and regex patterns are examples and
may require adjustment as the page markup changes.

## Building

```
mkdir build
cd build
cmake ..
make
```

Run the application with `./price_tracker`.
