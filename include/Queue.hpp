#pragma once

template <typename Data, int CAP>
class Queue {
	private:
		int size = 0;
		Data arr[CAP];
		int front = 0;
		int rear = 0;
	
	public:
		Queue<Data, CAP>(){}

		bool full(){
			return size == CAP-1;
		}

		bool empty(){
			return size == 0;
		}

		void push_back(Data sample){
			if(full()) return;
			arr[rear] = sample;
			rear = (rear + 1) % CAP;
			size++;
		}

		void pop_back(){
			if(empty()) return;
			rear = (rear - 1 + CAP) % CAP;
			size--;
		}

		void push_front(Data sample){
			if(full()) return;
			front = (front - 1 + CAP) % CAP;
			arr[front] = sample;
			size++;
		}

		void pop_front(){
			if(empty()) return;
			front = (front + 1) % CAP;
			size--;
		}

		Data getFront(){
			return arr[front];
		}

		Data getBack(){
			return arr[rear];
		}

};
