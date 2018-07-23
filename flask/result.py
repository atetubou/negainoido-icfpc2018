import flask
from typing import TypeVar,Generic
from abc import ABC, abstractmethod

T = TypeVar('T')
class Result(Generic[T], ABC):
    @abstractmethod
    def jsonify(self):
        raise NotImplementedError()

    @abstractmethod
    def render_template(self, template : str):
        raise NotImplementedError()

class Ok(Result[T]):
    def __init__(self, value : T) -> None:
        self.value = value
    def jsonify(self):
        return flask.jsonify({"result": "success", "value": self.value})
    
    def render_template(self, template):
        return flask.render_template(template, data = self.value)

class Ng(Result[T]):
    def __init__(self, reason : str = "") -> None:
        self.reason = reason
    def jsonify(self):
        return flask.jsonify({"result": "error", "reason": self.reason})
    def render_template(self, template):
        response = flask.render_template('ng.html', reason = self.reason) 
        return response, 400

