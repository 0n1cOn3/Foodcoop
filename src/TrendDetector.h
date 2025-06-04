#pragma once
#include <QObject>
#include "PriceFetcher.h"

class TrendDetector : public QObject
{
    Q_OBJECT
public:
    explicit TrendDetector(QObject *parent = nullptr);
    QString detectTrend(const QList<PriceEntry> &prices) const;
};
