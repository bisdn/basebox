#!/usr/bin/python

from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm.exc import NoResultFound
from sqlalchemy.orm import sessionmaker
from sqlalchemy import create_engine
from sqlalchemy import *




engine = create_engine('sqlite:///datapath.db', echo=True)
Base = declarative_base()


class Psi(Base):
    __tablename__ = 'psi'
    
    id = Column(Integer, Sequence('psi_id_seq'), primary_key = True)  
    name = Column(String(64), nullable=False)
    
    def __init__(self, name):
        self.name = name
        
    def __repr__(self):
        return "<PSI('%s')>" % (self.name)


class Lsi(Base):
    __tablename__ = 'lsi'
    
    id = Column(Integer, Sequence('lsi_id_seq'), primary_key = True)
    dpid = Column(Integer, nullable = False)
    psi_id = Column(Integer, ForeignKey("psi.id"), nullable = False)

    def __init__(self, dpid, psi_id):
        self.dpid = dpid
        self.psi_id = psi_id
        
    def __repr__(self):
        return "<LSI('%s', '%s')>" % (self.dpid, self.psi_id)
    
    
class LsiPort(Base):
    __tablename__ = 'lsiport'
    
    id = Column(Integer, Sequence('lsiport_id_seq'), primary_key = True)
    name = Column(String(16), nullable = False)
    hwaddr = Column(String(18), nullable = False)
    portno = Column(Integer, nullable = False)
    lsi_id = Column(Integer, ForeignKey("lsi.id"), nullable = False)
    
    def __init__(self, name, hwaddr, portno, lsi_id):
        self.name = name
        self.hwaddr = hwaddr
        self.portno = portno
        self.lsi_id = lsi_id
    
    def __repr__(self):
        return "<LsiPort('%s', '%s', '%s', %s')>" % (self.name, self.hwaddr, self.portno, self.lsi_id)
    
    
class VirtualMachine(Base):
    __tablename__ = 'vm'
    
    id = Column(Integer, Sequence('vm_id_seq'), primary_key = True)
    name = Column(String(32), nullable = False)
    ip_addr = Column(String(16), nullable = False)
    tcp_port = Column(Integer, nullable = False)
    lsi_id = Column(Integer, ForeignKey("lsi.id"), nullable = False) 
    
    def __init__(self, name, ip_addr, tcp_port, lsi_id):
        self.name = name
        self.ip_addr = ip_addr
        self.tcp_port = tcp_port
        self.lsi_id = lsi_id
    
    def __repr__(self):
        return "<VirtualMachine('%s', '%s', '%s', '%s')>" % (self.name, self.ip_addr, self.tcp_port, self.lsi_id)
    
    
class VirtualMachinePort(Base):
    __tablename__ = 'vmport'
    
    id = Column(Integer, Sequence('vmport_seq_id'), primary_key = True)
    name = Column(String(16), nullable = False)
    lsi_port_id = Column(Integer, ForeignKey("lsiport.id"), nullable = False, unique = True)
    vm_id = Column(Integer, ForeignKey("vm.id"), nullable = False)
    
    def __init__(self, name, lsi_port_id, vm_id):
        self.name = name
        self.lsi_port_id = lsi_port_id
        self.vm_id = vm_id
    
    def __repr__(self):
        return "<VirtualMachinePort('%s', '%s', '%s')>" % (self.name, self.lsi_port_id, self.vm_id)
     
    
    
Base.metadata.create_all(engine) 



    
def main():
    Session = sessionmaker(bind=engine)
    session = Session()
    
    try:
        session.query(Psi).filter(Psi.name == 'my PSI').one()
    except NoResultFound, e:
        session.add(Psi("my PSI"))
        session.commit()
        
    my_psi = session.query(Psi).filter(Psi.name == 'my PSI').one()
    
    
    try:
        session.query(Lsi).filter(Lsi.dpid == '1544').one()
    except NoResultFound, e:    
        session.add(Lsi('1544', my_psi.id))
        session.commit()
        
    my_lsi = session.query(Lsi).filter(Lsi.dpid == '1544').one()
    

    #lsiports = { 'eth1': {'hwaddr':'00:11:11:11:11:11', 'portno':'1'}, 'eth2': {'hwaddr':'00:22:22:22:22:22', 'portno':'2'}, 'eth3': {'hwaddr':'00:33:33:33:33:33', 'portno':'3'}, 'eth4': {'hwaddr':'00:44:44:44:44:44','portno':'4'} }
    lsiports = { 'ge0': {'hwaddr':'00:11:11:11:11:11', 'portno':'1'}, 'ge1': {'hwaddr':'00:22:22:22:22:22', 'portno':'2'}, 'ge2': {'hwaddr':'00:33:33:33:33:33', 'portno':'3'}, 'ge3': {'hwaddr':'00:44:44:44:44:44','portno':'4'} }
    for name in sorted(lsiports.iterkeys()):
        try:
            session.query(LsiPort).filter(LsiPort.name == name).one()
        except NoResultFound, e:
            session.add(LsiPort(name, lsiports[name]['hwaddr'], lsiports[name]['portno'], my_lsi.id))
    session.commit()


    try:
        session.query(VirtualMachine).one()
    except NoResultFound, e:
        session.add(VirtualMachine("vm1", "10.47.53.14", 6633, my_lsi.id))
        session.commit()
    
    my_vm = session.query(VirtualMachine).filter(Lsi.dpid == '1544').one()
    
    
    #vmports = { 'eth1': 'vnet1', 'eth2':'vnet2', 'eth3':'vnet3', 'eth4':'vnet4' }
    vmports = { 'ge0': 'vge0', 'ge1':'vge1', 'ge2':'vge2', 'ge3':'vge3' }        
    for name in sorted(vmports.iterkeys()):
        try:
            session.query(VirtualMachinePort).join(LsiPort).join(Lsi).join(Psi).\
                    filter(LsiPort.name==name).one()
        except NoResultFound, e:
            session.add(VirtualMachinePort(vmports[name], session.query(LsiPort).join(Lsi).join(Psi).filter(LsiPort.name==name).one().id, my_vm.id))
    session.commit()
    
    
    
    
    
if __name__ == "__main__":
    main()
    
    
    
    