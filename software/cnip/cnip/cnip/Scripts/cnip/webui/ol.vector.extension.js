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
            if (features[i].get(propertyname)) {
                if (features[i].get(propertyname).toString() === propertyvalue.toString()) {
                    matchedfeature = features[i]; break;
                }
            }
        }
    }
    return matchedfeature;
};
ol.source.Vector.prototype.getFeatureByProperties = function (properties) {
    const array = Object.keys(properties).map(property => ({ key: property, value: properties[property] }));
    let matchedfeature = undefined;
    if (array.length > 0) {
        let features = this.getFeatures();
        for (let i = 0; i < features.length; i++) {
            let matched = 0;
            for (let j = 0; j < array.length; j++) {
                if (features[i].get(array[j].key)) {
                    if (array[j].value) {
                        if (features[i].get(array[j].key).toString() === array[j].value.toString()) {
                            matched += 1;
                        }
                    }
                }
            }
            if (matched === array.length) {
                matchedfeature = features[i]; break;
            }
        }
    }
    return matchedfeature;
};
ol.source.Vector.prototype.getFeaturesByProperty = function (propertyname, propertyvalue) {
    let matchedfeatures = [];
    if (propertyname && propertyvalue) {
        let features = this.getFeatures();
        for (let i = 0; i < features.length; i++) {
            if (features[i].get(propertyname)) {
                if (Array.isArray(propertyvalue)) {
                    let matched = 0;
                    propertyvalue.forEach(function (value) {
                        if (value) {
                            if (features[i].get(propertyname).toString() === value.toString()) {
                                matched += 1;
                            }
                        }
                    });
                    if (matched > 0) {
                        matchedfeatures.push(features[i]);
                    }
                } else {
                    if (features[i].get(propertyname).toString() === propertyvalue.toString()) {
                        matchedfeatures.push(features[i]);
                    }
                }
            }
        }
    }
    return matchedfeatures;
};
ol.source.Vector.prototype.getFeaturesByProperties = function (properties) {
    const array = Object.keys(properties).map(property => ({ key: property, value: properties[property] }));
    let matchedfeatures = [];
    if (array.length > 0) {
        let features = this.getFeatures();
        for (let i = 0; i < features.length; i++) {
            let matched = 0;
            for (let j = 0; j < array.length; j++) {
                if (features[i].get(array[j].key)) {
                    if (Array.isArray(array[j].value)) {
                        let matchedOr = 0;
                        array[j].value.forEach(function (value) {
                            if (value) {
                                if (features[i].get(array[j].key).toString() === value.toString()) {
                                    matchedOr += 1;
                                }
                            }
                        });
                        if (matchedOr > 0) {
                            matched += 1;
                        }
                    } else {
                        if (array[j].value) {
                            if (features[i].get(array[j].key).toString() === array[j].value.toString()) {
                                matched += 1;
                            }
                        }
                    }
                }
            }
            if (matched === array.length) {
                matchedfeatures.push(features[i]);
            }
        }
    }
    return matchedfeatures;
};
ol.source.Vector.prototype.clearFeatures = function () {
    while (this.getFeatures().length > 0) {
        let features = this.getFeatures();
        for (let i = 0; i < features.length; i++) {
            this.removeFeature(features[i]);
        }
    }
};