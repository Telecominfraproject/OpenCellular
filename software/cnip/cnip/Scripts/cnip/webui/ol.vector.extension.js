ol.source.Vector.prototype.clone = function () {
    let JsonFeatures = new ol.format.GeoJSON().writeFeaturesObject(
        this.getFeatures(),
        { featureProjection: 'EPSG:3857', dataProjection: 'EPSG:4326' });
    let source = new ol.source.Vector({
        features: new ol.format.GeoJSON({
            extractStyles: true,
            extractAttributes: true
        }).readFeatures(JsonFeatures),
        wrapX: false
    });
    return source;
};
ol.source.Vector.prototype.getJSONString = function (beautify = false) {
    let JSONString;
    if (beautify) {
        JSONString = JSON.stringify(
            new ol.format.GeoJSON().writeFeaturesObject(
                this.getFeatures(),
                { featureProjection: 'EPSG:3857', dataProjection: 'EPSG:4326' }
            ), undefined, '\t'
        );
    } else {
        JSONString = JSON.stringify(
            new ol.format.GeoJSON().writeFeaturesObject(
                this.getFeatures(),
                { featureProjection: 'EPSG:3857', dataProjection: 'EPSG:4326' }
            )
        );
    }
    return JSONString;
};
ol.source.Vector.prototype.getKMLString = function (beautify = false) {
    let KMLString;
    if (beautify) {
        KMLString =
            JSON.stringify(
                new ol.format.KML({
                    extractStyles: true,
                    showPointNames: true,
                    writeStyles: true
                }).writeFeatures(
                    this.getFeatures(),
                    { featureProjection: 'EPSG:3857', dataProjection: 'EPSG:4326' }
                ), undefined, '\t'
            );
    } else {
        KMLString =
            JSON.stringify(
                new ol.format.KML({
                    extractStyles: true,
                    showPointNames: true,
                    writeStyles: true
                }).writeFeatures(
                    this.getFeatures(),
                    { featureProjection: 'EPSG:3857', dataProjection: 'EPSG:4326' }
                )
            );
    }
    KMLString = KMLString.replace(/\\/g, '');
    KMLString = KMLString.slice(1, -1);
    return KMLString;
};
ol.source.Vector.prototype.getFeatureByProperty = function (propertyname, propertyvalue) {
    let matchedfeature = undefined;
    if (propertyname && propertyvalue) {
        let features = this.getFeatures();
        for (let i = 0; i < features.length; i++) {
            if (features[i].get(propertyname).toString() === propertyvalue.toString()) {
                matchedfeature = features[i]; break;
            }
        }
    }
    return matchedfeature;
};
ol.source.Vector.prototype.clearFeatures = function () {
    while (this.getFeatures().length > 0) {
        let features = this.getFeatures();
        for (let i = 0; i < features.length; i++) {
            this.removeFeature(features[i]);
        }
    }
};