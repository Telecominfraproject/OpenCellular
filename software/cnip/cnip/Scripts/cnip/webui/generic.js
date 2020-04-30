var pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0;
setDraggable();
function setDraggable() {
    $('.draggable-handler').mousedown(function (e) {
        drag = $(this).closest('.draggable');
        drag.addClass('dragging');
        e = e || window.event;
        e.preventDefault();
        pos3 = e.clientX;
        pos4 = e.clientY;
        $(this).on('mousemove', function (e) {
            e = e || window.event;
            e.preventDefault();
            pos1 = pos3 - e.clientX;
            pos2 = pos4 - e.clientY;
            pos3 = e.clientX;
            pos4 = e.clientY;
            drag.css('top', $(this).offset().top - pos2 - /* border width*/3);
            drag.css('left', $(this).offset().left - pos1 - /* border width*/3);
            window.getSelection().removeAllRanges();
        });
    });
    $('.draggable-handler').mouseleave(stopDragging);
    $('.draggable-handler').mouseup(stopDragging);
}
function stopDragging(e) {
    if (e) { e.stopPropagation(); }
    drag = $(this).closest('.draggable');
    drag.removeClass('dragging');
    $(this).off('mousemove');
}
function openPage(e, pageName, elmnt) {
    if (e) { e.stopPropagation(); }
    var i, tabContent, tabLinks;
    tabContent = document.getElementsByClassName('pageTabContent');
    for (i = 0; i < tabContent.length; i++) {
        tabContent[i].style.display = 'none';
    }
    tabLinks = document.getElementsByClassName('pageTabLink');
    for (i = 0; i < tabLinks.length; i++) {
        tabLinks[i].style.backgroundColor = '';
        tabLinks[i].style.color = '';
    }
    document.getElementById(pageName).style.display = 'block';
    elmnt.style.backgroundColor = '#555';
    elmnt.style.color = 'white';
}
function round(value, precision) {
    return +(Math.round(value + 'e+' + precision) + 'e-' + precision);
}
function colorWithAlpha(color, alpha) {
    const [r, g, b] = Array.from(ol.color.asArray(color));
    return ol.color.asString([r, g, b, alpha]);
}
function sleep(milliseconds) {
    const date = Date.now();
    let currentDate = null;
    do {
        currentDate = Date.now();
    } while (currentDate - date < milliseconds);
}
var browser = (function (agent) {
    switch (true) {
        case agent.indexOf('edge') > -1: return 'edge';
        case agent.indexOf('edg') > -1: return 'chromium based edge (dev or canary)';
        case agent.indexOf('opr') > -1 && !!window.opr: return 'opera';
        case agent.indexOf('chrome') > -1 && !!window.chrome: return 'chrome';
        case agent.indexOf('trident') > -1: return 'ie';
        case agent.indexOf('firefox') > -1: return 'firefox';
        case agent.indexOf('safari') > -1: return 'safari';
        default: return 'other';
    }
})(window.navigator.userAgent.toLowerCase());
// digits obly check
const digits_only = string => [...string].every(c => '.0123456789'.includes(c));
// number check
function isANumber(str) {
    return !/\D/.test(str);
}
// isnumeric
var isNumber = function (value) { return /^\d+\.\d+$/.test(value); };
// convert to title case
function titleCase(str) {
    if (str) {
        return str.toString().toLowerCase().split(' ').map(function (word) {
            return word.replace(word[0], word[0].toUpperCase());
        }).join(' ');
    } else {
        return '';
    }
}
function fixStringName(stringName) {
    if (stringName) {
        return stringName.toString().replace(/[^A-Za-z0-9\s!?]/g, '');
    } else {
        return '';
    }
}

function allLetter(inputtxt) {
    var letters = /^[A-Za-z]+$/;
    if (inputtxt.toString().match(letters)) {
        return true;
    }
    else {
        return false;
    }
}

var truncate = function (str, length) {
    if (str.length > length) {
        return str.substr(0, length - 3) + '...';
    } else {
        return str;
    }
};


/* no ui slider extension with value span control */
function initSingleSlider(target, width, minValue, maxValue, stepSize, defaultValue) {
    if (!target.hasChildNodes()) {
        let sliTable = document.createElement('table');
        let sliTbody = document.createElement('tbody');
        let sliTr = document.createElement('tr');
        let sliTd1 = document.createElement('td');
        let sliTd2 = document.createElement('td');
        let sliDiv = document.createElement('div');
        let sliSpan = document.createElement('span');
        sliTd1.appendChild(sliDiv);
        sliTd2.appendChild(sliSpan);
        sliTr.appendChild(sliTd1);
        sliTr.appendChild(sliTd2);
        sliTbody.appendChild(sliTr);
        sliTable.appendChild(sliTbody);
        target.appendChild(sliTable);
        sliTable.id = target.id + '_table';
        sliDiv.id = target.id + '_slider';
        sliSpan.id = target.id + '_span';
        sliTable.style.width = '100%';
        sliTable.style.marginBottom = '5px';
        sliTd1.style.width = '85%';
        sliTd1.style.verticalAlign = 'middle';
        sliTd2.style.width = '15%';
        sliTd2.style.verticalAlign = 'middle';
        sliSpan.style.backgroundColor = 'var(--facecolor)';
        sliSpan.style.display = 'flex';
        sliSpan.style.padding = '5px 10px 5px 10px';
        sliSpan.style.marginLeft = '17px';
        sliSpan.style.justifyContent = 'center';
        sliSpan.style.width = '50px';
        sliSpan.style.fontSize = 'x-small';
        sliSpan.style.borderRadius = '4px';
    }
    let table_ = document.getElementById(target.id + '_table');
    let slider_ = document.getElementById(target.id + '_slider');
    let span_ = document.getElementById(target.id + '_span');
    if (width !== '') {
        table_.style.width = width;
    }
    span_.innerHTML = round(parseFloat(defaultValue), 1);
    target.value = parseFloat(defaultValue);
    noUiSlider.create(slider_, {
        start: parseFloat(defaultValue),
        connect: true,
        step: parseFloat(stepSize),
        range: {
            'min': parseFloat(minValue),
            'max': parseFloat(maxValue)
        }
    });
    slider_.noUiSlider.on('change', function (values, handle) {
        let value = values[handle];
        span_.innerHTML = round(value, 1);
        target.value = value;
        if ('createEvent' in document) {
            let evt = document.createEvent('Event');
            evt.initEvent('change', true, true);
            target.dispatchEvent(evt);
        } else {
            target.fireEvent('onchange');
        }
    });
    slider_.noUiSlider.on('set', function (values, handle) {
        let value = values[handle];
        span_.innerHTML = round(value, 1);
        target.value = value;
    });
    target.setValue = function (value) {
        if (this.hasChildNodes) {
            if (this.firstChild.id.includes('_table')) {
                this.firstChild.firstChild.firstChild.firstChild.firstChild.noUiSlider.set(value);
            }
        }
    };
}
/* Select Option no ui control */
function initSelectOpt(target, width, lstOpt, defaultValue) {
    if (!target.hasChildNodes()) {
        let sopButton = document.createElement('button');
        let sopDiv = document.createElement('div');
        target.appendChild(sopButton);
        target.appendChild(sopDiv);
        sopButton.id = target.id + '_selectBar';
        sopButton.className = 'selectBar collapsed';
        sopButton.setAttribute('data-toggle', 'collapse');
        sopDiv.id = target.id + '_selectDiv';
        sopDiv.className = 'selectDiv collapse';
    }
    let selectBar = target.firstChild;
    let selectDiv = selectBar.nextElementSibling;
    let dataset = [];
    lstOpt.split('\t').forEach(function (item, index) {
        dataset.push({ columnname: item, datatype: 'str', class: 'datatypstr' });
    });
    if (width !== '') {
        selectBar.style.width = width;
    }
    selectBar.dataset.target = '#' + selectDiv.id;
    target.value = '';
    selectBar.innerHTML = '<i class="fa fa-caret-down" style="float: right"></i>';
    selectDiv.innerHTML = '';
    for (let i = 0; i < dataset.length; i++) {
        selectDiv.innerHTML += '<div class="selectList" onclick="this.parentElement.choose(event);" data-value="'
            + dataset[i].columnname + '">' + dataset[i].columnname + '</div>';
        if (dataset[i].columnname === defaultValue) {
            target.value = defaultValue;
            selectBar.innerHTML = defaultValue + '<i class="fa fa-caret-down" style="float: right"></i>';
        }
    }
    if (target.value === '' && dataset.length > 0) {
        target.value = dataset[0].columnname;
        selectBar.innerHTML = dataset[0].columnname + '<i class="fa fa-caret-down" style="float: right"></i>';
    }
    selectBar.hide = function (e) {
        if (e) { e.stopPropagation(); }
        let selectBar = e.currentTarget;
        let selectDiv = selectBar.nextElementSibling;
        if (selectDiv.mouseIsOver === false) {
            selectDiv.className = 'selectDiv collapse';
            selectBar.className = 'selectBar collapsed';
        }
    };
    selectBar.focus = function (e) {
        if (e) { e.stopPropagation(); }
        let selectBar = e.currentTarget;
        let selectDiv = selectBar.nextElementSibling;
        selectDiv.style.width = selectBar.offsetWidth + 'px';
    };
    selectDiv.choose = function (e) {
        if (e) { e.stopPropagation(); }
        let target = e.currentTarget.parentElement.parentElement;
        let selectBar = e.currentTarget.parentElement.previousElementSibling;
        let selectDiv = e.currentTarget.parentElement;
        target.value = e.currentTarget.dataset.value;
        selectBar.innerHTML = e.currentTarget.dataset.value + '<i class="fa fa-caret-down" style="float: right"></i>';
        selectDiv.className = 'selectDiv collapse';
        selectBar.className = 'selectBar collapsed';
        if ('createEvent' in document) {
            var evt = document.createEvent('Event');
            evt.initEvent('change', true, true);
            target.dispatchEvent(evt);
        } else {
            target.fireEvent('onchange');
        }
    };
    selectBar.addEventListener('focus', selectBar.focus);
    selectBar.addEventListener('focusout', selectBar.hide);
    selectDiv.mouseIsOver = false;
    selectDiv.onmouseover = () => {
        selectDiv.mouseIsOver = true;
    };
    selectDiv.onmouseout = () => {
        selectDiv.mouseIsOver = false;
    };
    target.setOpt = function (value) {
        if (this.hasChildNodes) {
            if (this.firstChild.className.includes('selectBar') && this.firstChild.id.includes('selectBar')) {
                this.firstChild.innerHTML = value + '<i class="fa fa-caret-down" style="float: right"></i>';
                this.value = value;
            }
        }
    };
    target.reloadOpt = function (value) {
        if (this.hasChildNodes) {
            if (this.firstChild.className.includes('selectBar') && this.firstChild.id.includes('selectBar')) {
                initSelectOpt(this, '', value, '');
            }
        }
    };
    selectBar.addEventListener('click', function () {
        $(selectBar.getAttribute('data-target')).collapse('toggle');
    });
}