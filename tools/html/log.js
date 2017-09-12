// Copyright (c) 2017 Akos Kiss.
//
// Licensed under the BSD 3-Clause License
// <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
// This file may not be copied, modified, or distributed except
// according to those terms.

function Log(node) {
    this.node = node;
}

Log.prototype.print = function(str) {
    this.node.textContent += str;
}

Log.prototype.println = function(str='') {
    this.print(str + '\n');
}

Log.prototype.clear = function() {
    this.node.textContent = '';
}
