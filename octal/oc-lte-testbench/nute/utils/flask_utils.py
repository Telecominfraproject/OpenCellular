# coding=utf8

from flask import g, request, jsonify

def shutdown_flask_server():
    func = request.environ.get('werkzeug.server.shutdown')
    if func is None:
        raise RuntimeError('Not running with the Werkzeug Server')
    func()

def dto_json_api(blueprint, dto_object, methods=['GET', 'POST'], url_head=''):

    @blueprint.route(url_head + '/', defaults={'path': ''}, methods=methods)
    @blueprint.route(url_head + '/<path:path>', methods=methods)
    def catch_all(path):
        path = [sub_path for sub_path in path.split('/')]
        dto = dto_object
        if request.method == 'GET':
            return jsonify(dto.get_path(path))
        elif request.method == 'POST':
            dto.update(path=path, data=request.get_json())
            return jsonify(dto.get_path(path))

def socketio_subscribe_dto_stream(socketio, dto, event_name='update', room_name='', sub_path=[]):
    """
    Streams the dto updates to the room_name of the socketio instance with
    event_name as the message name.

    The event names are the path joined with underscores.
    dto path ['foo', 'bar'] -> emit(event_name,  {path=['foo', 'bar'], content={content}}, room=room_name)
    dto path []             -> emit(event_name,  {path=[], content={content}}, room=room_name)

    javascript (client) side with angularjs:
    socket.on({{event_name}}, function(json) {
        if(json.path.length == 0){
            $scope.dto = json.content;
        }
        else{
            var obj = $scope.dto;
            for(let i = 0, l = data.path.length; i < l - 1; i++){
                obj = obj[data.path[i]]
            }
            obj[data.path[i]] = json.content
        }
    });


    :param socketio: the object which will be used for emit
    :param dto: The DataTransferObject to observe
    :param event_name: The event name which will be used.
    :param room_name: The room name where the emits will be sent.
    """
    def callback(event):
        socketio.emit(event_name, {'path': sub_path + event.path, 'content': event.content, 'op': event.op}, room_name=room_name)

    dto.subscribe(callback)
