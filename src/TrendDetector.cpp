#include "TrendDetector.h"
#include <QtMath>

TrendDetector::TrendDetector(QObject *parent)
    : QObject(parent)
{
}

QString TrendDetector::detectTrend(const QList<PriceEntry> &prices) const
{
    if (prices.size() < 2)
        return "stable";

    double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    int n = prices.size();
    for (int i = 0; i < n; ++i) {
        double x = i;
        double y = prices[i].price;
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }
    double slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX + 1e-9);
    if (slope > 0.01)
        return "upward";
    if (slope < -0.01)
        return "downward";
    return "stable";
}
