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
