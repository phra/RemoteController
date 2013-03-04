package com.phtest.remotecontroller;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutput;
import java.io.ObjectOutputStream;
import java.nio.ByteBuffer;


public class Pacco {
	private int type, len;
	private byte[] payload;
	
	public Pacco(int type, byte[] payload, int len){
		this.type = type;
		this.payload = payload;
		this.len = len;
	}
	
	public byte[] getData(){
		return payload;
	}
	
	public int getType(){
		return type;
	}
	
	public int getSize(){
		return len;
	}

	public byte[] getSerializedHeader() {
		ByteArrayOutputStream baos = new ByteArrayOutputStream();
		DataOutputStream w = new DataOutputStream(baos);
		
		try {
			w.writeInt(len);
			w.writeInt(type);
			w.flush();
		} catch (IOException e) {
			return null;
		}
		return baos.toByteArray();
	}
	
	public byte[] getSerialized() {
	ByteArrayOutputStream baos = new ByteArrayOutputStream();
	DataOutputStream w = new DataOutputStream(baos);

	try {
		w.writeInt(len);
		w.writeInt(type);
		w.write(payload);
		w.flush();
	} catch (IOException e) {
		return null;
	}
	return baos.toByteArray();
}
	
/*
	public byte[] getSerialized(){
		byte[] bytes;
		ByteBuffer bb = ByteBuffer.allocate(payload.length+8);
		bb.putInt(len).putInt(type).put(payload).flip();
		bytes = new byte[bb.remaining()];
		bb.get(bytes);
		return bytes;
	}
*/
	private byte[] intToBytes(int my_int) throws IOException {
	    ByteArrayOutputStream bos = new ByteArrayOutputStream();
	    ObjectOutput out = new ObjectOutputStream(bos);
	    out.writeInt(my_int);
	    out.close();
	    byte[] int_bytes = bos.toByteArray();
	    bos.close();
	    return int_bytes;
	}
	
	private int bytesToInt(byte[] int_bytes) throws IOException {
	    ByteArrayInputStream bis = new ByteArrayInputStream(int_bytes);
	    ObjectInputStream ois = new ObjectInputStream(bis);
	    int my_int = ois.readInt();
	    ois.close();
	    return my_int;
	}
}
