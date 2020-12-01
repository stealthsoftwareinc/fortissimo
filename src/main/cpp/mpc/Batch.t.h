/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<FF_TYPENAMES>
::std::string Batch<FF_TYPES>::name() {
  if (this->children.empty()) {
    return ::std::string("Batch size: 0");
  }
  return ::std::string("Batch size: ") +
      ::std::to_string(this->children.size()) + " of " +
      this->children[0]->name();
}

template<FF_TYPENAMES>
bool Batch<FF_TYPES>::checkFirstLastActionTypes(
    ::std::deque<::std::vector<::std::unique_ptr<Action<FF_TYPES>>>> &
        actions) {
  if (actions.size() <= 1) {
    return true;
  }

  ::std::vector<::std::unique_ptr<Action<FF_TYPES>>> & first =
      actions.front();
  ::std::vector<::std::unique_ptr<Action<FF_TYPES>>> & last =
      actions.back();

  if (first.size() != last.size()) {
    return false;
  }

  for (size_t i = 0; i < first.size(); i++) {
    if (first[i]->type != last[i]->type) {
      return false;
    }
    if (first[i]->type == ActionType::invoke &&
        (static_cast<InvokeAction<FF_TYPES> &>(*first[i]).promised !=
             static_cast<InvokeAction<FF_TYPES> &>(*last[i]).promised ||
         !(static_cast<InvokeAction<FF_TYPES> &>(*first[i]).peers ==
           static_cast<InvokeAction<FF_TYPES> &>(*last[i]).peers))) {
      return false;
    }
    if (first[i]->type == ActionType::send &&
        static_cast<SendAction<FF_TYPES> &>(*first[i]).msg->recipient !=
            static_cast<SendAction<FF_TYPES> &>(*last[i])
                .msg->recipient) {
      return false;
    }
  }

  return true;
}

template<FF_TYPENAMES>
void Batch<FF_TYPES>::handleActions(
    ::std::deque<::std::vector<::std::unique_ptr<Action<FF_TYPES>>>> &
        actions) {
  for (size_t i = 0; i < actions[0].size(); i++) {
    auto sub_actions = actions.begin();

    ActionType type = sub_actions->at(i)->type;

    if (type == ActionType::send) {
      ::std::unique_ptr<OutgoingMessage_T> omsg(new OutgoingMessage_T(
          static_cast<SendAction<FF_TYPES> &>(*sub_actions->at(i))
              .msg->recipient));

      // check batch size matches peer's batch size.
      {
        omsg->template write<uint64_t>((uint64_t)this->children.size());
      }

      while (sub_actions != actions.end()) {
        omsg->template write<OutgoingMessage_T>(
            *static_cast<SendAction<FF_TYPES> &>(*sub_actions->at(i))
                 .msg);
        static_cast<SendAction<FF_TYPES> &>(*sub_actions->at(i))
            .msg->clear();

        sub_actions++;
      }

      this->send(std::move(omsg));
    } else if (type == ActionType::invoke) {
      if (static_cast<InvokeAction<FF_TYPES> &>(*sub_actions->at(i))
              .promised) {
        log_error("Batch promises not supported.");
        this->abort();
        return;
      }

      ::std::vector<::std::unique_ptr<Fronctocol<FF_TYPES>>>
          sub_children;

      while (sub_actions != actions.end()) {
        sub_children.push_back(::std::move(
            static_cast<InvokeAction<FF_TYPES> &>(*sub_actions->at(i))
                .fronctocol));

        sub_actions++;
      }

      this->invoke(
          ::std::unique_ptr<Fronctocol<FF_TYPES>>(
              new Batch<FF_TYPES>(::std::move(sub_children))),
          static_cast<InvokeAction<FF_TYPES> &>(*actions[0][i]).peers);
    } else if (type == ActionType::complete) {
      this->complete();
      /* allow a child class (inheritance) to do cleanup */
      this->onComplete();
    } else if (type == ActionType::abortion) {
      this->abort();
      return;
    } else if (type == ActionType::await) {
      log_error("Batch promises are not supported");
      this->abort();
      return;
    } else {
      log_error("Invalid action type.");
      this->abort();
      return;
    }
  }
}

template<FF_TYPENAMES>
Batch<FF_TYPES>::Batch() {
}

template<FF_TYPENAMES>
Batch<FF_TYPES>::Batch(
    ::std::vector<::std::unique_ptr<Fronctocol<FF_TYPES>>> && cs) :
    children(std::move(cs)) {
}

template<FF_TYPENAMES>
void Batch<FF_TYPES>::init() {
  /* Allow a child class (inheritance) to do start up tasks */
  this->onInit();

  this->batchHandler = ::std::unique_ptr<FronctocolHandler<FF_TYPES>>(
      new FronctocolHandler<FF_TYPES>(
          this->getSelf(), this->getPeers()));
  this->batchHandler->id = this->getId();

  ::std::deque<::std::vector<::std::unique_ptr<::ff::Action<FF_TYPES>>>>
      actions;

  for (::std::unique_ptr<Fronctocol<FF_TYPES>> & child :
       this->children) {
    child->setHandler(this->batchHandler.get());

    actions.emplace_back();

    child->setActions(&actions.back());
    child->init();
    child->setActions(nullptr);

    if_debug if (!this->checkFirstLastActionTypes(actions)) {
      log_error("Differing action types in Batch");
      this->abort();
      return;
    }
  }

  this->handleActions(actions);
}

template<FF_TYPENAMES>
void Batch<FF_TYPES>::handleReceive(IncomingMessage_T & imsg) {
  ::std::deque<::std::vector<::std::unique_ptr<::ff::Action<FF_TYPES>>>>
      actions;

  // check batch size matches peer's batch size.
  {
    uint64_t num_peer_children = 0;
    imsg.template read<uint64_t>(num_peer_children);

    if ((size_t)num_peer_children != this->children.size()) {
      log_error(
          "Batch (%zu children) has differing length from"
          " peer (%zu children)",
          this->children.size(),
          (size_t)num_peer_children);
      this->abort();
    }
  }

  for (::std::unique_ptr<Fronctocol<FF_TYPES>> & child :
       this->children) {
    actions.emplace_back();

    child->setActions(&actions.back());
    child->handleReceive(imsg);
    child->setActions(nullptr);

    if_debug if (!this->checkFirstLastActionTypes(actions)) {
      log_error("Differing action types in batched Batch");
      this->abort();
      return;
    }
  }

  this->handleActions(actions);
}

template<FF_TYPENAMES>
void Batch<FF_TYPES>::handleComplete(Fronctocol<FF_TYPES> & f) {
  Batch<FF_TYPES> & sub_batch = static_cast<Batch<FF_TYPES> &>(f);

  ::std::deque<::std::vector<::std::unique_ptr<::ff::Action<FF_TYPES>>>>
      actions;

  for (size_t i = 0; i < this->children.size(); i++) {
    ::std::unique_ptr<Fronctocol<FF_TYPES>> & child = this->children[i];

    actions.emplace_back();

    child->setActions(&actions.back());
    child->handleComplete(*sub_batch.children[i]);
    child->setActions(nullptr);

    if_debug if (!this->checkFirstLastActionTypes(actions)) {
      log_error("Differing action types in Batch");
      this->abort();
      return;
    }
  }

  this->handleActions(actions);
}

template<FF_TYPENAMES>
void Batch<FF_TYPES>::handlePromise(Fronctocol<FF_TYPES> &) {
  log_error("Batch promises are not implemented");
  this->abort();
}

} // namespace mpc
} // namespace ff
